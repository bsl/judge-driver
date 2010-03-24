#ifndef INCLUDED_OPTIONS_H
#define INCLUDED_OPTIONS_H

#include <stdio.h>  // FILE

#include "result.h"

typedef struct {
    unsigned int max_user_time_in_seconds;
    unsigned int max_cpu_time_in_seconds;
    unsigned int max_memory_in_bytes;
} resource_limits_t;

typedef struct {
    char *  program_path;
    char ** program_arguments;
    char ** program_environment;
    resource_limits_t resource_limits;
} options_t;

result_t options_get(int, char ** const, options_t * const);
void options_display(FILE * const, const options_t * const);

#endif
