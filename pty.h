#ifndef INCLUDED_PTY_H
#define INCLUDED_PTY_H

#include "result.h"

result_t pty_open_raw(int * const, int * const);
result_t pty_prepare_for_login(const int);

#endif
