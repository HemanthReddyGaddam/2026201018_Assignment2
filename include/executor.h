#ifndef EXECUTOR_H
#define EXECUTOR_H

// Hands off non-builtin commands to fork() and execvp()
void executesystemcmd(char** args, int argc, bool bg);

#endif