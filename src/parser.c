#include <stdio.h>
#include <string.h>

#include "parser.h"

static void command_init(command_t *cmd)
{
    cmd->argc = 0;

    cmd->input[0] = '\0';

    cmd->output[0] = '\0';

    cmd->append = 0;

    cmd->background = 0;

    for(int i=0;i<MAX_ARGS;i++)
        cmd->argv[i] = NULL;
}

int parser(token_list_t *tokens,
          pipeline_t *pipeline)
{
    pipeline->command_count = 1;

    int current = 0;

    command_init(&pipeline->commands[0]);

    for(int i=0;i<tokens->count;i++)
    {
        token_t *t = &tokens->tokens[i];

        switch(t->type)
        {

        case TOKEN_WORD:

            pipeline->commands[current]
                .argv[pipeline->commands[current].argc++] =
                    t->text;

            break;

        case TOKEN_INPUT:

            if(tokens->tokens[i+1].type != TOKEN_WORD)
            {
                printf("Parser Error: filename expected after <\n");
                return 0;
            }

            strcpy(pipeline->commands[current].input,
                   tokens->tokens[++i].text);

            break;

        case TOKEN_OUTPUT:

            if(tokens->tokens[i+1].type != TOKEN_WORD)
            {
                printf("Parser Error: filename expected after >\n");
                return 0;
            }

            strcpy(pipeline->commands[current].output,
                   tokens->tokens[++i].text);

            pipeline->commands[current].append = 0;

            break;

        case TOKEN_APPEND:

            if(tokens->tokens[i+1].type != TOKEN_WORD)
            {
                printf("Parser Error: filename expected after >>\n");
                return 0;
            }

            strcpy(pipeline->commands[current].output,
                   tokens->tokens[++i].text);

            pipeline->commands[current].append = 1;

            break;

        case TOKEN_BACKGROUND:

            pipeline->commands[current].background = 1;

            break;

        case TOKEN_PIPE:

            pipeline->commands[current]
                .argv[pipeline->commands[current].argc] = NULL;

            current++;

            if(current >= MAX_COMMANDS)
            {
                printf("Too many commands in pipeline.\n");
                return 0;
            }

            command_init(&pipeline->commands[current]);

            pipeline->command_count++;

            break;

        case TOKEN_END:

            break;
        }
    }

    pipeline->commands[current]
        .argv[pipeline->commands[current].argc] = NULL;

    return 1;
}

void pipeline_print(const pipeline_t *pipeline)
{
    printf("\n========== PIPELINE ==========\n");

    for(int i=0;i<pipeline->command_count;i++)
    {
        command_t cmd = pipeline->commands[i];

        printf("\nCommand %d\n", i+1);
        printf("-----------------------------\n");

        printf("Arguments\n");

        for(int j=0;j<cmd.argc;j++)
            printf("argv[%d] = %s\n", j, cmd.argv[j]);

        printf("Input      : %s\n",
               strlen(cmd.input) ? cmd.input : "None");

        printf("Output     : %s\n",
               strlen(cmd.output) ? cmd.output : "None");

        printf("Append     : %s\n",
               cmd.append ? "Yes" : "No");

        printf("Background : %s\n",
               cmd.background ? "Yes" : "No");
    }

    printf("==============================\n");
}
