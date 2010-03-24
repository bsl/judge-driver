#include <stdlib.h>  // getenv

#include "result.h"

result_t env_get(const char * const name, char ** value)
{
    *value = getenv(name);
    succeed_or_fail(*value != NULL);
}
