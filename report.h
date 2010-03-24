#ifndef DEFINED_REPORT_H
#define DEFINED_REPORT_H

#include <errno.h>  // errno

void report_error_(const char *, const char *, const int, const char *, const int);
void report_failure_(const char *, const char *, const int, const char *, ...);

#define report_error(function) \
    report_error_(__FILE__, __FUNCTION__, __LINE__, function, errno)

#define report_failure(format...) \
    report_failure_(__FILE__, __FUNCTION__, __LINE__, format)

#endif
