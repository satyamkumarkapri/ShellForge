#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "builtin.h"
#include "executor.h"


/*
 * Execute one command.
 *
 * Returns:
 *      0  -> success
 *     -1  -> error
 */
int execute_command(command_t *cmd)
{
    pid_t pid;
    int status;


    /*
     * Check whether the command is valid.
     */
    if (cmd == NULL || cmd->argc == 0)
    {
        return -1;
    }


    /*
     * ------------------------------------------------
     * Check for built-in command
     * ------------------------------------------------
     *
     * Built-ins such as cd, pwd, echo and exit
     * are handled by the shell itself.
     */
    if (is_builtin(cmd))
    {
        return execute_builtin(cmd);
    }


    /*
     * ------------------------------------------------
     * Create a child process
     * ------------------------------------------------
     */
    pid = fork();


    /*
     * fork() failed
     */
    if (pid < 0)
    {
        perror("fork");
        return -1;
    }


    /*
     * ------------------------------------------------
     * CHILD PROCESS
     * ------------------------------------------------
     */
    if (pid == 0)
    {
        /*
         * execvp() expects:
         *
         * argv[0] = command
         * argv[1] = argument
         * ...
         * argv[argc] = NULL
         */

        char *args[MAX_ARGS + 1];


        /*
         * Convert our 2D character array into
         * an array of pointers.
         */
        for (int i = 0; i < cmd->argc; i++)
        {
            args[i] = cmd->argv[i];
        }


        /*
         * execvp() requires NULL termination.
         */
        args[cmd->argc] = NULL;


        /*
         * Replace the child process with
         * the requested external command.
         */
        execvp(args[0], args);


        /*
         * If execvp() returns, execution failed.
         */
        perror("Shellforge");

        exit(EXIT_FAILURE);
    }


    /*
     * ------------------------------------------------
     * PARENT PROCESS
     * ------------------------------------------------
     */

    /*
     * Wait for the child process to finish.
     */
    if (waitpid(pid, &status, 0) == -1)
    {
        perror("waitpid");
        return -1;
    }


    /*
     * Check how the child terminated.
     */
    if (WIFEXITED(status))
    {
        /*
         * Return child's exit status.
         */
        return WEXITSTATUS(status);
    }


    /*
     * Child terminated abnormally.
     */
    if (WIFSIGNALED(status))
    {
        fprintf(stderr,
                "Process terminated by signal %d\n",
                WTERMSIG(status));

        return -1;
    }


    return 0;
}
