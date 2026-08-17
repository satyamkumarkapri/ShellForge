#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "history.h"
#include "token.h"
#include "lexer.h"

/* Function to print command history */
void print_history_list(void)
{
    HIST_ENTRY **hist = history_list();

    if (hist == NULL)
    {
        printf("No history available.\n");
        return;
    }

    printf("\n------ Command History ------\n");

    for (int i = 0; hist[i] != NULL; i++)
    {
        printf("%d. %s\n", i + 1, hist[i]->line);
    }

    printf("-----------------------------\n");
}

int main(void)
{
    printf("=====================================\n");
    printf("          ShellForge\n");
    printf("   A Unix Style Shell Written in C\n");
    printf("=====================================\n");

    token_list_t tokens;

    using_history();

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

        /* Exit command */
        if (strcmp(line, "exit") == 0)
        {
            free(line);
            printf("Exiting ShellForge...\n");
            break;
        }

        /* Show history */
        if (strcmp(line, "history") == 0)
        {
            print_history_list();
            free(line);
            continue;
        }

        /* Save command to history */
        add_history(line);

        /* Lex command */
        lexer(line, &tokens);

        /* Print tokens */
        token_print(&tokens);

        free(line);
    }

    return 0;
}
