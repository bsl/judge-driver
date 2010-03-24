#ifndef INCLUDED_EXITCODE_H
#define INCLUDED_EXITCODE_H

/* judge-driver should always exit with one of EXITCODE_* or 128+n where n is
 * the number of the signal that was fatal to the solution program.
 *
 * The EXITCODE_* values were chosen to resemble FTP/HTTP 3-digit response
 * codes.
 */
typedef enum {
    EXITCODE_PROGRAM_EXITED           = 40,
    EXITCODE_USER_TIME_LIMIT_EXCEEDED = 41,
    EXITCODE_CPU_TIME_LIMIT_EXCEEDED  = 42,

    EXITCODE_USAGE_ERROR              = 50,
    EXITCODE_INTERNAL_ERROR           = 51,
} exitcode_t;

#define exitcode_for_fatal_signal(n) (128+n)

#endif
