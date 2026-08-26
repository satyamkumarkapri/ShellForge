#ifndef BUILTIN_H
#define BUILTIN_H

#include "parser.h"

/*
 * Check whether a command is a built-in command.
 *
 * Returns:
 *      1 -> built-in
 *      0 -> external command
 */
int is_builtin(const command_t *cmd);


/*
 * Execute a built-in command.
 *
 * Returns:
 *      0 -> command executed successfully
 *      1 -> shell should exit
 *     -1 -> error
 */
int execute_builtin(command_t *cmd);

#endif
