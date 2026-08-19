#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <readline/history.h>
#include <readline/readline.h>
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "expand.h"
int main(void)
{
    // Display a welcome banner when the shell starts
    printf("=====================================\n");
    printf("      Shellforge \n");
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

         add_history(line);
	lexer(line, &tokens);
        token_print(&tokens);
        

	if(parser(&tokens, &pipeline))
	{
		expand_variables(&pipeline);
    		pipeline_print(&pipeline);
	}
        


        if (strcmp(line, "exit") == 0)
        {
            free(line);
            printf("Exiting...\n");
            break;
        }
	free(line);
    }    
    return 0;
}
