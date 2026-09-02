#ifndef BUILTIN_CD_H
#define BUILTIN_CD_H
//store the previous directory path
extern char PREV_DIR[];
// passing  the arguments  to shift form  old to new directory
void execute_cd(char** args, int arg_count);

#endif