#ifndef COMMANDS_H
#define COMMANDS_H

#include "ast.h"

bool command(const std::string &input, FunctionTable &functions, Variables &variables);

#endif