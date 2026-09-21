#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <readline/history.h>
#include <readline/readline.h>
#include "history.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "expand.h"
#include "builtin.h"
#include "executor.h"

int main(void)
{
    // Display a welcome banner when the shell starts
    printf("=====================================\n");
    printf("      Shellforge \n");
    printf(" A Unix Style Shell written in C\n");
    printf("=====================================\n");
 using_history();
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

       if (strcmp(line, "history") == 0)
       {
          print_history();
          free(line);
           continue;
       }
// milestone 1 - enabling history

        add_history(line);

// milestone 2.1 - tokenization and lexer

	lexer(line, &tokens);

        // token_print(&tokens);

// milestone 2.2 - expansion of environment variables and parser

	if(parser(&tokens, &pipeline))
	{
		expand_variables(&pipeline);
    	//	pipeline_print(&pipeline);
	}


	if (pipeline.command_count == 1 &&  pipeline.commands[0].argc > 0 && strcmp(pipeline.commands[0].argv[0],"exit") == 0)
         {
                free(line);
                break;
            }

        execute_pipeline(&pipeline);

       free(line);

    }
    return 0;
}
