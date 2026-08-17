#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "lexer.h"

void lexer(const char *input, token_list_t *list)
{
    token_list_init(list);

    int i = 0;

    while (input[i] != '\0')
    {
        /* Skip whitespace */
        if (isspace((unsigned char)input[i]))
        {
            i++;
            continue;
        }

        /* Pipe */
        if (input[i] == '|')
        {
            token_add(list, TOKEN_PIPE, "|");
            i++;
            continue;
        }

        /* Input redirection */
        if (input[i] == '<')
        {
            token_add(list, TOKEN_INPUT, "<");
            i++;
            continue;
        }

        /* Output redirection */
        if (input[i] == '>')
        {
            if (input[i + 1] == '>')
            {
                token_add(list, TOKEN_APPEND, ">>");
                i += 2;
            }
            else
            {
                token_add(list, TOKEN_OUTPUT, ">");
                i++;
            }

            continue;
        }

        /* Background */
        if (input[i] == '&')
        {
            token_add(list, TOKEN_BACKGROUND, "&");
            i++;
            continue;
        }

        /* Build one WORD token */
        char word[MAX_TOKEN_LEN];
        int j = 0;

        while (input[i] != '\0')
        {
            char c = input[i];

            /* End of word */
            if (isspace((unsigned char)c) ||
                c == '|' ||
                c == '<' ||
                c == '>' ||
                c == '&')
            {
                break;
            }

            /* Single Quote */
            if (c == '\'')
            {
                i++;

                while (input[i] != '\0' && input[i] != '\'')
                {
                    if (j < MAX_TOKEN_LEN - 1)
                    {
                        word[j++] = input[i];
                    }

                    i++;
                }

                /* Unterminated single quote */
                if (input[i] != '\'')
                {
                    printf("Lexer Error: Unterminated single quote\n");
                    return;
                }

                i++;
                continue;
            }

            /* Double Quote */
            if (c == '"')
            {
                i++;

                while (input[i] != '\0' && input[i] != '"')
                {
                    /* Escaped character inside double quotes */
                    if (input[i] == '\\' && input[i + 1] != '\0')
                    {
                        i++;
                    }

                    if (j < MAX_TOKEN_LEN - 1)
                    {
                        word[j++] = input[i];
                    }

                    i++;
                }

                /* Unterminated double quote */
                if (input[i] != '"')
                {
                    printf("Lexer Error: Unterminated double quote\n");
                    return;
                }

                i++;
                continue;
            }

            /* Escape Character */
            if (c == '\\')
            {
                i++;

                if (input[i] != '\0')
                {
                    if (j < MAX_TOKEN_LEN - 1)
                    {
                        word[j++] = input[i];
                    }

                    i++;
                }

                continue;
            }

            /* Normal Character */
            if (j < MAX_TOKEN_LEN - 1)
            {
                word[j++] = c;
            }

            i++;
        }

        /* Finish the word */
        word[j] = '\0';

        token_add(list, TOKEN_WORD, word);
    }

    /* End token */
    token_add(list, TOKEN_END, "END");
}
