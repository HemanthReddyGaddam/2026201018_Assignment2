#ifndef BUILTIN_CD_H
#define BUILTIN_CD_H
// Keep track of where we were so "cd -" doesn't panic
extern char prev_dir[];
// Handles changing directories around the system
void executecd(char** args, int argc);

#endif
