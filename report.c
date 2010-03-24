#define _POSIX_C_SOURCE 200905L  // XSI-compliant strerror_r
#define _XOPEN_SOURCE   200905L  // vsnprintf

#include <stdio.h>   // fprintf
#include <stdarg.h>  // vsnprintf
#include <string.h>  // strerror_r

#define ERROR_FORMAT   "%s, %s(), line %d: %s: %s\n"
#define FAILURE_FORMAT "%s, %s(), line %d: %s\n"

void report_error_(
    const char * filename,
    const char * functionname,
    const int    line_number,
    const char * error_functionname,
    const int    error_number
) {
    char buf[256];

    if (strerror_r(error_number, buf, sizeof buf) == 0)
        fprintf(stderr, ERROR_FORMAT, filename, functionname, line_number, error_functionname, buf);
}

void report_failure_(
    const char * filename,
    const char * functionname,
    const int    line_number,
    const char * format2,
    ...
) {
    va_list ap;
    char buf1[128], buf2[256];

    va_start(ap, format2);
    vsnprintf(buf1, sizeof buf1, format2, ap);
    va_end(ap);

    snprintf(buf2, sizeof buf2, FAILURE_FORMAT, filename, functionname, line_number, buf1);
    fputs(buf2, stderr);
}
