#ifndef PROMPT_H
#define PROMPT_H

#include <limits.h>

//here we note the global variable for the shell home 
//this become our static home reference point represented by '~'
extern char SHELL_HOME[PATH_MAX];

//capture and store the shell home directory path into SHELL_HOME
void init_shell_home();
//display the username, hostname, and the modified current path
void displayprompt();

#endif