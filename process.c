#define _POSIX_SOURCE   // kill

#include <unistd.h>     // fork
#include <sys/types.h>  // kill waitpid
#include <signal.h>     // kill
#include <sys/wait.h>   // waitpid

#include "result.h"

result_t process_create(pid_t * const pid)
{
    *pid = fork();
    succeed_or_fail_due_to_error(*pid != -1, "fork");
}

result_t process_destroy(const pid_t pid)
{
    if (kill(pid, SIGKILL) == -1)
        fail_due_to_error("kill");

    if (waitpid(pid, NULL, 0) == -1)
        fail_due_to_error("waitpid");

    succeed();
}
