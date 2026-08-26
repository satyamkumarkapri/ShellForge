#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/history.h>
#include <readline/readline.h>

#include "history.h"
#include "token.h"
#include "lexer.h"
#include "builtin.h"
#include "parser.h"
#include "expand.h"
#include "executor.h"

int main(void)
{
    printf("=====================================\n");
    printf("      Shellforge\n");
    printf(" A Unix Style Shell written in C\n");
    printf("=====================================\n");

    token_list_t tokens;
    pipeline_t pipeline;

    char *line;

    while (1)
    {
        line = readline("shellforge$ ");

        if (line == NULL)
        {
            printf("\nGoodbye!\n");
            break;
        }

        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }

        /* History command */
        if (strcmp(line, "history") == 0)
        {
            print_history();
            free(line);
            continue;
        }

        /* Add command to history */
        add_history(line);

        /* Lexer */
        lexer(line, &tokens);

        /* Parser and expansion */
        if (parser(&tokens, &pipeline))
        {
            expand_variables(&pipeline);
        }

        /* Execute commands */
        for (int i = 0; i < pipeline.command_count; i++)
        {
            int result = execute_command(&pipeline.commands[i]);

            if (result == 1)
            {
                free(line);
                return 0;
            }
        }

        free(line);
    }

    return 0;
}
