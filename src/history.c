#include <stdio.h>
#include <readline/history.h>

void print_history(void)
{
    HIST_ENTRY **list = history_list();

    if (list == NULL)
    {
        printf("History is empty.\n");
        return;
    }

    printf("\n------- Command History -------\n");

    for (int i = 0; list[i] != NULL; i++)
    {
        printf("%2d %s\n", i + history_base, list[i]->line);
    }

    printf("-------------------------------\n");
}
