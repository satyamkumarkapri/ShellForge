#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "executor.h"
#include "builtin.h"


/* =========================================================
   EXECUTE SINGLE COMMAND
   ========================================================= */

int execute_command(command_t *cmd)
{
    pid_t pid;
    int status;

    if (cmd == NULL || cmd->argc == 0)
    {
        return -1;
    }


    /* Built-in command */

    if (is_builtin(cmd))
    {
        return execute_builtin(cmd);
    }


    /* Create child */

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return -1;
    }


    /* Child process */

    if (pid == 0)
    {
        char *args[MAX_ARGS + 1];

        for (int i = 0; i < cmd->argc; i++)
        {
            args[i] = cmd->argv[i];
        }

        args[cmd->argc] = NULL;


        execvp(args[0], args);


        perror(args[0]);

        _exit(127);
    }


    /* Parent process */

    if (waitpid(pid, &status, 0) < 0)
    {
        perror("waitpid");
        return -1;
    }


    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }


    return -1;
}


/* =========================================================
   EXECUTE PIPELINE
   ========================================================= */

int execute_pipeline(pipeline_t *pipeline)
{
    int previous_read = -1;

    pid_t pids[MAX_COMMANDS];

    int command_count;


    if (pipeline == NULL)
    {
        return -1;
    }


    command_count = pipeline->command_count;


    if (command_count == 0)
    {
        return -1;
    }


    /*
     * If there is only one command,
     * execute it normally.
     */

    if (command_count == 1)
    {
        return execute_command(&pipeline->commands[0]);
    }


    /*
     * Multi-command pipeline
     */

    for (int i = 0; i < command_count; i++)
    {
        int pipefd[2];


        /*
         * Create pipe unless this is
         * the last command.
         */

        if (i < command_count - 1)
        {
            if (pipe(pipefd) == -1)
            {
                perror("pipe");
                return -1;
            }
        }


        /*
         * Create child process
         */

        pids[i] = fork();

        if (pids[i] < 0)
        {
            perror("fork");
            return -1;
        }


        /* ================================================
           CHILD PROCESS
           ================================================ */

        if (pids[i] == 0)
        {
            command_t *cmd =
                &pipeline->commands[i];


            /*
             * --------------------------------------------
             * INPUT REDIRECTION
             * --------------------------------------------
             *
             * If this is not the first command,
             * receive input from the previous pipe.
             */

            if (previous_read != -1)
            {
                if (dup2(previous_read,
                         STDIN_FILENO) == -1)
                {
                    perror("dup2 input");
                    _exit(EXIT_FAILURE);
                }
            }


            /*
             * --------------------------------------------
             * OUTPUT REDIRECTION
             * --------------------------------------------
             *
             * If this is not the last command,
             * send output into the current pipe.
             */

            if (i < command_count - 1)
            {
                if (dup2(pipefd[1],
                         STDOUT_FILENO) == -1)
                {
                    perror("dup2 output");
                    _exit(EXIT_FAILURE);
                }
            }


            /*
             * Close inherited descriptors.
             */

            if (previous_read != -1)
            {
                close(previous_read);
            }


            if (i < command_count - 1)
            {
                close(pipefd[0]);
                close(pipefd[1]);
            }


            /*
             * Convert Shellforge argv
             * into execvp argument format.
             */

            char *args[MAX_ARGS + 1];

            for (int j = 0;
                 j < cmd->argc;
                 j++)
            {
                args[j] = cmd->argv[j];
            }

            args[cmd->argc] = NULL;


            /*
             * Built-ins inside a pipe.
             *
             * Note:
             * cd inside a pipeline will only
             * affect this child process.
             */

            if (is_builtin(cmd))
            {
                int result =
                    execute_builtin(cmd);

                _exit(result == 0 ? 0 : 1);
            }


            /*
             * Execute external command.
             */

            execvp(args[0], args);


            perror(args[0]);

            _exit(127);
        }


        /* ================================================
           PARENT PROCESS
           ================================================ */


        /*
         * Parent no longer needs
         * the previous pipe read end.
         */

        if (previous_read != -1)
        {
            close(previous_read);
        }


        /*
         * Parent keeps the read end of the
         * current pipe for the next command.
         */

        if (i < command_count - 1)
        {
            close(pipefd[1]);

            previous_read = pipefd[0];
        }
        else
        {
            previous_read = -1;
        }
    }


    /*
     * Wait for every child process.
     */

    int final_status = 0;

    for (int i = 0; i < command_count; i++)
    {
        int status;

        if (waitpid(pids[i],
                    &status,
                    0) < 0)
        {
            perror("waitpid");
            continue;
        }


        /*
         * Save status of the last command.
         */

        if (i == command_count - 1)
        {
            if (WIFEXITED(status))
            {
                final_status =
                    WEXITSTATUS(status);
            }
        }
    }


    return final_status;
}
