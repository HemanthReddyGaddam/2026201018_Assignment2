#ifndef PROMPT_H
#define PROMPT_H

#include <limits.h>

// Stores where the shell was launched so we can print ~ instead of full paths
extern char shell_home[PATH_MAX];

void initshellhome();
void displayprompt();

#endif