#ifndef DEFINED_RESULT_H
#define DEFINED_RESULT_H

#include <errno.h>

#include "report.h"

typedef enum { FAILURE = 0, SUCCESS = 1 } result_t;

#define fail()    return FAILURE
#define succeed() return SUCCESS

#define fail_with_message(format...) ({                        \
    report_failure_(__FILE__, __FUNCTION__, __LINE__, format); \
    fail();                                                    \
})

#define fail_due_to_error(function) ({                                \
    report_error_(__FILE__, __FUNCTION__, __LINE__, function, errno); \
    fail();                                                           \
})

#define succeed_or_fail(cond) \
    return (cond) ? SUCCESS : FAILURE

#define succeed_or_fail_due_to_error(cond, function) ({ \
    if (cond)                                           \
        return SUCCESS;                                 \
    fail_due_to_error(function);                        \
})

#endif
