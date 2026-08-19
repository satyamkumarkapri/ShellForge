#ifndef EXPAND_H
#define EXPAND_H

#include "parser.h"

/* Expand environment variables in argv[] */
void expand_variables(pipeline_t *pipeline);

#endif

