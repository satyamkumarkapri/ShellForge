#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

int execute_command(command_t *cmd);

int execute_pipeline(pipeline_t *pipeline);

void reap_background_processes(void);

void setup_background_handler(void);
#endif
