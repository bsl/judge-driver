#include <stdlib.h>  // strtoul
#include <errno.h>   // errno

#include "result.h"

result_t string_to_unsigned_int(const char * const s, unsigned int * const ui)
{
    char *se;

    if (s == NULL || s[0] == '\0')
        fail();

    errno = 0;
    *ui = strtoul(s, &se, 10);
    succeed_or_fail(*se == '\0' && !errno);
}
