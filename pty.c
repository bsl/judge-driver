#define _POSIX_C_SOURCE 200905L  // posix_openpt
#define _XOPEN_SOURCE            // grantpt unlockpt
#define _GNU_SOURCE              // ptsname_r

#include <stdlib.h>    // grantpt posix_openpt ptsname_r unlockpt
#include <unistd.h>    // cfmakeraw close dup2 setsid tcsetattr
#include <sys/stat.h>  // open
#include <fcntl.h>     // open posix_openpt
#include <limits.h>    // PATH_MAX
#include <stropts.h>   // ioctl
#include <termios.h>   // cfmakeraw tcsetattr
#include <pty.h>       // TIOCSCTTY

#include "pty.h"
#include "result.h"

result_t pty_open_raw(int * const master_fd, int * const slave_fd)
{
    char slave_filename[PATH_MAX];
    struct termios tio;

    *master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (*master_fd == -1)
        fail_due_to_error("posix_openpt");

    if (grantpt(*master_fd) == -1)
        fail_due_to_error("grantpt");

    if (unlockpt(*master_fd) == -1)
        fail_due_to_error("unlockpt");

    if (ptsname_r(*master_fd, slave_filename, sizeof slave_filename) != 0)
        fail_due_to_error("ptsname_r");

    *slave_fd = open(slave_filename, O_RDWR | O_NOCTTY);
    if (*slave_fd == -1)
        fail_due_to_error("open");

    cfmakeraw(&tio);

    if (tcsetattr(*slave_fd, TCSAFLUSH, &tio) == -1)
        fail_due_to_error("tcsetattr");

    succeed();
}

/* Prepare for a login on fd. fd may be a tty or the slave of a pty as returned
 * by pty_open_raw().
 *
 * See openpty(3), login_tty().
 */
result_t pty_prepare_for_login(const int fd)
{
    // Create a new session.
    if (setsid() == -1)
        fail_due_to_error("setsid");

    // Make fd the controlling terminal.
    if (ioctl(fd, TIOCSCTTY, NULL) == -1)
        fail_due_to_error("ioctl");

    // Duplicate fd to stdin, stdout, and stderr.
    if (
        (dup2(fd, 0) == -1) ||
        (dup2(fd, 1) == -1) ||
        (dup2(fd, 2) == -1)
    )
        fail_due_to_error("dup2");

    // Close fd.
    if (fd > 2)
        close(fd);

    succeed();
}
