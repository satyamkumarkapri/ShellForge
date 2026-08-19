#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expand.h"

void expand_variables(pipeline_t *pipeline)
{
    for(int i = 0; i < pipeline->command_count; i++)
    {
        command_t *cmd = &pipeline->commands[i];

        for(int j = 0; j < cmd->argc; j++)
        {
            char *arg = cmd->argv[j];

            /* Ignore NULL arguments */
            if(arg == NULL)
                continue;

            /* Environment variable ? */
            if(arg[0] == '$')
            {
                char *name = arg + 1;

                char *value = getenv(name);

                if(value != NULL)
                {
                    cmd->argv[j] = value;
                }
                else
                {
                    /*
                     * Undefined variables become
                     * an empty string (similar to Bash)
                     */
                    cmd->argv[j] = "";
                }
            }
        }
    }
}
