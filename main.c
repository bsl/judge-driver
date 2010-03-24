/* 2009.05.29
 *
 * judge-driver executes a given program under a pseudo terminal session and
 * serves as a bidirectional proxy, relaying stdin and stdout, allowing the
 * caller to interact with the child. It also allows the caller to specify the
 * child's argument list, environment, and restrict the time and memory the
 * child is allowed to use.
 *
 * judge-driver expects its argv to be the argv of the child (see exec(3),
 * execve).
 *
 * Additionally, 3 environment variables must be set:
 *
 *   * JD_MAX_CPU_TIME:
 *     Unsigned integer number of seconds. If the child is still running after
 *     consuming the given number of seconds of CPU time, it will be killed by
 *     the kernel and judge-driver will exit.
 *
 *   * JD_MAX_USER_TIME:
 *     Unsigned integer number of seconds. If the child is still running after
 *     the given number of seconds, judge-driver will kill it and exit. This
 *     number should almost certainly be larger than JD_MAX_CPU_TIME.
 *
 *   * JD_MAX_MEM:
 *     Unsigned integer number of bytes. If the child attempts to allocate more
 *     memory than this number, it will fail.
 *
 * By default, the child's environment will be empty. To specify entries, set
 * variables in judge-driver's environment with prefix JD_ENV_ like
 *
 *   JD_ENV_XXX=YYY
 *   JD_ENV_ABC=123
 *   JD_ENV_KEY=VALUE
 *
 * Done this way, the child's environment will be
 *
 *   XXX=YYY
 *   ABC=123
 *   KEY=VALUE
 *
 * judge-driver should always exit in one of the following situations and exit
 * codes:
 *   * the child exits normally                        40
 *   * the user time limit is exceeded                 41
 *   * the CPU time limit is exceeded                  42
 *   * the child is killed by unexpected signal n      128+n
 *   * incorrect parameters are given to judge-driver  50
 *   * an internal error occurs                        51
 */

#include <stdlib.h>        // exit
#include <unistd.h>        // close execve read sleep write
#include <sys/resource.h>  // setrlimit
#include <sys/select.h>    // select fd_set FD_ZERO FD_SET FD_ISSET
#include <wait.h>          // wait SIGKILL

#include "exitcode.h"
#include "options.h"
#include "process.h"
#include "pty.h"

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

static result_t start_relay(pid_t * const, const int, const int);
static void     run_relay(const int);
static result_t relay(const int, const int, unsigned int * const);

static result_t start_timer(pid_t * const, const int, const int, const unsigned int);
static void     run_timer(const unsigned int);

static result_t start_child(pid_t * const, const int, const int, const options_t * const);
static void     run_child(const int, const options_t * const);

static result_t ignore_sighup();
static result_t set_resource_limits(const resource_limits_t * const);
static result_t set_resource_limit(const int, const rlim_t);

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

int main(int argc, char ** argv)
{
    options_t options;
    int master_fd, slave_fd;
    pid_t relay_pid, timer_pid, solution_pid, pid;
    int status;

    if (!options_get(argc, argv, &options))
        return EXITCODE_USAGE_ERROR;

    options_display(stderr, &options);

    /* Open a pseudo terminal. Start the child in a new process; it uses the
     * pseudo terminal for its stdin, stdout, and stderr. Start a process whose
     * job it is to relay data bidirectionally between our stdin/out and the
     * child's. Start a timer to enforce the user time limit.
     */
    if (
        !pty_open_raw(&master_fd, &slave_fd)                       ||
        !start_child(&solution_pid, master_fd, slave_fd, &options) ||
        !start_relay(&relay_pid, master_fd, slave_fd)              ||
        !start_timer(&timer_pid, master_fd, slave_fd, options.resource_limits.max_user_time_in_seconds)
    )
        return EXITCODE_INTERNAL_ERROR;

    close(master_fd);
    close(slave_fd);

    /* Wait for a child to end, but wait again if the first we see happens to
     * be the relay (seeing the relay end is inconclusive). The real race is
     * between the timer and child processes.
     */
    do {
        pid = wait(&status);
        if (pid == -1)
            return EXITCODE_INTERNAL_ERROR;
    } while (pid == relay_pid);

    if (pid == timer_pid) {
        /* The timer ended first, which means the child took too much user
         * time.
         */
        process_destroy(solution_pid);
        process_destroy(relay_pid);
        return EXITCODE_USER_TIME_LIMIT_EXCEEDED;
    }

    if (pid == solution_pid) {
        /* The child ended first. If it exited normally, communicate that to
         * the caller.
         */
        if (WIFEXITED(status))
            return EXITCODE_PROGRAM_EXITED;

        /* It didn't exit normally. Maybe it got killed by the kernel with
         * SIGKILL for taking too much CPU time, or killed by some other signal
         * we didn't expect.
         */
        if (WIFSIGNALED(status))
            return WTERMSIG(status) == SIGKILL
                ? EXITCODE_CPU_TIME_LIMIT_EXCEEDED
                : exitcode_for_fatal_signal(WTERMSIG(status));
    }

    /* Something strange happened.
     */
    return EXITCODE_INTERNAL_ERROR;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

static result_t start_relay(pid_t * const pid, const int master_fd, const int slave_fd)
{
    if (!process_create(pid))
        fail();

    if (*pid == 0) {
        close(slave_fd);
        run_relay(master_fd);
    }

    succeed();
}

/* Relay data from stdin (0) to master_fd.
 * Relay data from master_fd to stdout (1).
 */
static void run_relay(const int master_fd)
{
    fd_set fds, tmp_fds;
    unsigned int n;

    FD_ZERO(&fds);
    FD_SET(0, &fds);
    FD_SET(master_fd, &fds);

    while (1) {
        tmp_fds = fds;

        if (select(master_fd + 1, &tmp_fds, NULL, NULL, NULL) == -1)
            break;

        if (FD_ISSET(0, &tmp_fds)) {
            if (!relay(0, master_fd, &n))
                break;

            if (n == 0) {
                /* stdin was closed by the caller. Relay the closure to the
                 * child.
                 */
                close(master_fd);
                break;
            }
        }

        if (FD_ISSET(master_fd, &tmp_fds))
            if (!relay(master_fd, 1, NULL))
                break;
    }

    exit(0);
}

/* Read up to 1kB of data from src and write it to dst. Assumes there is data
 * waiting to be read on src.
 */
static result_t relay(const int src, const int dst, unsigned int * const n_r)
{
    char buf[1024];
    ssize_t nr;

    nr = read(src, buf, sizeof buf);
    if (nr == -1)
        fail();

    if (nr > 0) {
        ssize_t nw = write(dst, buf, nr);
        if (nw == -1 || nw != nr)
            fail();
    }

    if (n_r != NULL)
        *n_r = nr;
    succeed();
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

static result_t start_timer(pid_t * const pid, const int master_fd, const int slave_fd, const unsigned int n)
{
    if (!process_create(pid))
        fail();

    if (*pid == 0) {
        close(master_fd);
        close(slave_fd);
        run_timer(n);
    }

    succeed();
}

static void run_timer(const unsigned int n)
{
    sleep(n);
    exit(0);
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

static result_t start_child(pid_t * const pid, const int master_fd, const int slave_fd, const options_t * const options)
{
    if (!process_create(pid))
        fail();

    if (*pid == 0) {
        close(master_fd);
        run_child(slave_fd, options);
    }

    succeed();
}

static void run_child(const int slave_fd, const options_t * const options)
{
    if (
        !pty_prepare_for_login(slave_fd)                ||
        !set_resource_limits(&options->resource_limits) ||
        !ignore_sighup()
    )
        exit(0);

    execve(
        options->program_path,
        options->program_arguments,
        options->program_environment
    );
    exit(0);
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

/* When the caller closes our stdin, we relay this to the child by closing the
 * master fd associated with its pseudo terminal. This causes SIGHUP to be sent
 * to the child. The child needs to ignore it, or it would end.
 */
static result_t ignore_sighup()
{
    struct sigaction sa;

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);

    succeed_or_fail_due_to_error(sigaction(SIGHUP, &sa, NULL) == 0, "sigaction");
}

static result_t set_resource_limits(const resource_limits_t * const rl)
{
    succeed_or_fail(
        set_resource_limit(RLIMIT_CORE, 0)                           &&  // disallow creation of core files
        set_resource_limit(RLIMIT_CPU,  rl->max_cpu_time_in_seconds) &&  // set maximum number of CPU seconds
        set_resource_limit(RLIMIT_AS,   rl->max_memory_in_bytes)         // set maximum memory
    );
}

/* Set the hard and soft limits of the given resource to the same value.
 */
static result_t set_resource_limit(const int resource, const rlim_t v)
{
    struct rlimit rl = { .rlim_cur = v, .rlim_max = v };

    succeed_or_fail_due_to_error(setrlimit(resource, &rl) != -1, "setrlimit");
}
