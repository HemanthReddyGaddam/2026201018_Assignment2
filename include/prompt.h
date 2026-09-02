#ifndef PROMPT_H
#define PROMPT_H

#include <limits.h>

extern char SHELL_HOME[PATH_MAX];

void init_shell_home();
void display_prompt();

#endif