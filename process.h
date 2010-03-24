#ifndef INCLUDED_PROCESS_H
#define INCLUDED_PROCESS_H

#include "result.h"

result_t process_create(pid_t * const);
result_t process_destroy(const pid_t);

#endif
