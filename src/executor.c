#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "parser.h"
#include "executor.h"
#include "builtin.h"


/* =========================================================
   BACKGROUND PROCESS HANDLER

   This handler prevents zombie processes.
   ========================================================= */

static void sigchld_handler(int sig)
{
    int saved_errno = errno;

    (void)sig;

    /*
     * Reap all finished child processes.
     */

    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
        /* Keep collecting finished children */
    }

    errno = saved_errno;
}


/* =========================================================
   INSTALL SIGCHLD HANDLER
   ========================================================= */

void setup_background_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = sigchld_handler;

    sigemptyset(&sa.sa_mask);

    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;


    if (sigaction(SIGCHLD, &sa, NULL) < 0)
    {
        perror("sigaction SIGCHLD");
    }
}


/* =========================================================
   APPLY I/O REDIRECTION
   ========================================================= */

static int apply_redirection(command_t *cmd)
{
    int fd;


    /* =====================================================
       INPUT REDIRECTION: <
       ===================================================== */

    if (cmd->input[0] != '\0')
    {
        fd = open(cmd->input, O_RDONLY);

        if (fd < 0)
        {
            perror(cmd->input);
            return -1;
        }


        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("dup2 input");

            close(fd);

            return -1;
        }

        close(fd);
    }


    /* =====================================================
       OUTPUT REDIRECTION: > and >>
       ===================================================== */

    if (cmd->output[0] != '\0')
    {
        /* ============================
           APPEND: >>
           ============================ */

        if (cmd->append)
        {
            fd = open(
                cmd->output,
                O_WRONLY | O_CREAT | O_APPEND,
                0644
            );
        }

        /* ============================
           OVERWRITE: >
           ============================ */

        else
        {
            fd = open(
                cmd->output,
                O_WRONLY | O_CREAT | O_TRUNC,
                0644
            );
        }


        if (fd < 0)
        {
            perror(cmd->output);

            return -1;
        }


        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2 output");

            close(fd);

            return -1;
        }


        close(fd);
    }


    return 0;
}


/* =========================================================
   EXECUTE A SINGLE COMMAND
   ========================================================= */

int execute_command(command_t *cmd)
{
    pid_t pid;

    int status;


    if (cmd == NULL)
    {
        return -1;
    }


    if (cmd->argc == 0)
    {
        return 0;
    }


    /* =====================================================
       BUILTIN COMMAND
       ===================================================== */

    if (is_builtin(cmd))
    {
        /*
         * Builtins such as cd must execute in the shell
         * process for foreground execution.
         */

        if (!cmd->background)
        {
            return execute_builtin(cmd);
        }
    }


    /* =====================================================
       CREATE CHILD
       ===================================================== */

    pid = fork();


    if (pid < 0)
    {
        perror("fork");

        return -1;
    }


    /* =====================================================
       CHILD PROCESS
       ===================================================== */

    if (pid == 0)
    {
        char *args[MAX_ARGS + 1];


        /* Apply < > >> */

        if (apply_redirection(cmd) < 0)
        {
            _exit(EXIT_FAILURE);
        }


        /* Prepare argv */

        for (int i = 0; i < cmd->argc; i++)
        {
            args[i] = cmd->argv[i];
        }

        args[cmd->argc] = NULL;


        /* =============================================
           BUILTIN IN CHILD

           Needed for background builtin commands.
           ============================================= */

        if (is_builtin(cmd))
        {
            int result = execute_builtin(cmd);

            _exit(result);
        }


        /* =============================================
           EXTERNAL COMMAND
           ============================================= */

        execvp(args[0], args);


        /* execvp returns only on failure */

        perror(args[0]);

        _exit(127);
    }


    /* =====================================================
       PARENT PROCESS
       ===================================================== */


    /* =====================================================
       BACKGROUND PROCESS
       ===================================================== */

    if (cmd->background)
    {
        printf("[Background PID: %d]\n", pid);

        /*
         * DO NOT wait.
         *
         * Shell immediately returns to the prompt.
         */

        return 0;
    }


    /* =====================================================
       FOREGROUND PROCESS
       ===================================================== */

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
    int command_count;

    int previous_read = -1;

    pid_t pids[MAX_COMMANDS];

    int status;

    int final_status = 0;

    int i;


    /* =====================================================
       VALIDATION
       ===================================================== */

    if (pipeline == NULL)
    {
        return -1;
    }


    command_count = pipeline->command_count;


    if (command_count <= 0)
    {
        return 0;
    }


    /* =====================================================
       CHECK BACKGROUND STATUS

       We use the last command because pipeline_t does not
       have a background member.
       ===================================================== */

    int background =
        pipeline->commands[command_count - 1].background;


    /* =====================================================
       SINGLE COMMAND
       ===================================================== */

    if (command_count == 1)
    {
        return execute_command(
            &pipeline->commands[0]
        );
    }


    /* =====================================================
       MULTI-STAGE PIPELINE
       ===================================================== */

    for (i = 0; i < command_count; i++)
    {
        int pipefd[2];


        /* =================================================
           CREATE PIPE

           No pipe is needed after the final command.
           ================================================= */

        if (i < command_count - 1)
        {
            if (pipe(pipefd) < 0)
            {
                perror("pipe");

                return -1;
            }
        }


        /* =================================================
           CREATE CHILD
           ================================================= */

        pids[i] = fork();


        if (pids[i] < 0)
        {
            perror("fork");

            return -1;
        }


        /* =================================================
           CHILD PROCESS
           ================================================= */

        if (pids[i] == 0)
        {
            command_t *cmd =
                &pipeline->commands[i];

            char *args[MAX_ARGS + 1];


            /* =============================================
               INPUT FROM PREVIOUS PIPE
               ============================================= */

            if (previous_read != -1)
            {
                if (dup2(
                        previous_read,
                        STDIN_FILENO
                    ) < 0)
                {
                    perror("dup2 previous pipe");

                    _exit(EXIT_FAILURE);
                }
            }


            /* =============================================
               OUTPUT TO NEXT PIPE
               ============================================= */

            if (i < command_count - 1)
            {
                if (dup2(
                        pipefd[1],
                        STDOUT_FILENO
                    ) < 0)
                {
                    perror("dup2 next pipe");

                    _exit(EXIT_FAILURE);
                }
            }


            /* =============================================
               CLOSE PREVIOUS PIPE
               ============================================= */

            if (previous_read != -1)
            {
                close(previous_read);
            }


            /* =============================================
               CLOSE CURRENT PIPE DESCRIPTORS
               ============================================= */

            if (i < command_count - 1)
            {
                close(pipefd[0]);

                close(pipefd[1]);
            }


            /* =============================================
               APPLY FILE REDIRECTION

               Redirection is applied after pipe setup.

               Example:

               cat file.txt | grep hello > output.txt
               ============================================= */

            if (apply_redirection(cmd) < 0)
            {
                _exit(EXIT_FAILURE);
            }


            /* =============================================
               PREPARE ARGUMENTS
               ============================================= */

            for (int j = 0; j < cmd->argc; j++)
            {
                args[j] = cmd->argv[j];
            }

            args[cmd->argc] = NULL;


            /* =============================================
               BUILTIN COMMAND INSIDE PIPELINE
               ============================================= */

            if (is_builtin(cmd))
            {
                int result =
                    execute_builtin(cmd);

                _exit(result);
            }


            /* =============================================
               EXECUTE EXTERNAL COMMAND
               ============================================= */

            execvp(args[0], args);


            perror(args[0]);


            _exit(127);
        }


        /* =================================================
           PARENT PROCESS
           ================================================= */


        /* Close previous read descriptor */

        if (previous_read != -1)
        {
            close(previous_read);
        }


        /* Keep read end for next command */

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


    /* =====================================================
       BACKGROUND PIPELINE
       ===================================================== */

    if (background)
    {
        printf(
            "[Background Pipeline PID: %d]\n",
            pids[0]
        );


        /*
         * Do not wait for children.
         *
         * SIGCHLD handler will clean them up.
         */

        return 0;
    }


    /* =====================================================
       FOREGROUND PIPELINE

       Wait for every process.
       ===================================================== */

    for (i = 0; i < command_count; i++)
    {
        status = 0;


        if (waitpid(pids[i], &status, 0) < 0)
        {
            perror("waitpid");

            continue;
        }


        /*
         * Pipeline exit status is normally taken from
         * the last command.
         */

        if (i == command_count - 1)
        {
            if (WIFEXITED(status))
            {
                final_status =
                    WEXITSTATUS(status);
            }

            else
            {
                final_status = -1;
            }
        }
    }


    return final_status;
}
