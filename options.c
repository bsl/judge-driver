#include <stdio.h>   // fprintf
#include <stdlib.h>  // malloc
#include <string.h>  // strncmp

#include "env.h"
#include "options.h"
#include "result.h"
#include "string.h"

#define JD_MAX_USER_TIME "JD_MAX_USER_TIME"
#define JD_MAX_CPU_TIME  "JD_MAX_CPU_TIME"
#define JD_MAX_MEM       "JD_MAX_MEM"

#define JD_ENV        "JD_ENV_"
#define JD_ENV_LENGTH strlen(JD_ENV)

#define ENV_VAR_NOT_SET "%s not set"
#define ENV_VAR_INVALID "%s invalid"

extern char ** environ;

result_t options_get(int argc, char ** argv, options_t * const options)
{
    char *v, **envp;
    unsigned int n, i;

    if (argc < 2)
        fail_with_message("program path not specified");
    options->program_path = argv[1];

    if (argc < 3)
        fail_with_message("program arguments not specified");
    options->program_arguments = argv + 2;

    if (!env_get(JD_MAX_USER_TIME, &v))
        fail_with_message(ENV_VAR_NOT_SET, JD_MAX_USER_TIME);

    if (!string_to_unsigned_int(v, &options->resource_limits.max_user_time_in_seconds))
        fail_with_message(ENV_VAR_INVALID, JD_MAX_USER_TIME);

    if (!env_get(JD_MAX_CPU_TIME, &v))
        fail_with_message(ENV_VAR_NOT_SET, JD_MAX_CPU_TIME);

    if (!string_to_unsigned_int(v, &options->resource_limits.max_cpu_time_in_seconds))
        fail_with_message(ENV_VAR_INVALID, JD_MAX_CPU_TIME);

    if (!env_get(JD_MAX_MEM, &v))
        fail_with_message(ENV_VAR_NOT_SET, JD_MAX_MEM);

    if (!string_to_unsigned_int(v, &options->resource_limits.max_memory_in_bytes))
        fail_with_message(ENV_VAR_INVALID, JD_MAX_MEM);

    /* Determine the number of elements in the child environment 'n' by
     * counting variables in our environment that have prefix JD_ENV.
     */
    for (n=0, envp=environ; *envp!=NULL; envp++)
        if (!strncmp(*envp, JD_ENV, JD_ENV_LENGTH))
            n++;

    /* Allocate space for n char pointers, plus one for null termination.
     */
    options->program_environment = malloc((n + 1) * sizeof (char *));
    if (options->program_environment == NULL)
        fail_due_to_error("malloc");

    /* Fill the array, skipping the JD_ENV prefix.
     * Example: JD_ENV_PATH=/bin
     *                 ^------->
     */
    for (i=0, envp=environ; *envp!=NULL && i<n; envp++) {
        if (!strncmp(*envp, JD_ENV, JD_ENV_LENGTH))
            options->program_environment[i++] = *envp + JD_ENV_LENGTH;
    }

    /* Terminate the array.
     */
    options->program_environment[n] = NULL;

    succeed();
}

void options_display(FILE * const stream, const options_t * const options)
{
    unsigned int i;

    fprintf(stream, "program path: '%s'\n", options->program_path);

    fprintf(stream, "program arguments:\n");
    for (i=0; options->program_arguments[i]!=NULL; i++)
        fprintf(stream, "  %02d: '%s'\n", i, options->program_arguments[i]);

    fprintf(stream, "program environment:\n");
    for (i=0; options->program_environment[i]!=NULL; i++)
        fprintf(stream, "  %02d: '%s'\n", i, options->program_environment[i]);

    fprintf(stream, "resource limits:\n");
    fprintf(stream, "  max user time: %ds\n",      options->resource_limits.max_user_time_in_seconds);
    fprintf(stream, "  max CPU time:  %ds\n",      options->resource_limits.max_cpu_time_in_seconds);
    fprintf(stream, "  max memory:    %d bytes\n", options->resource_limits.max_memory_in_bytes);
}
