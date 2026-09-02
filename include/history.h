#ifndef HISTORY_H
#define HISTORY_H

void init_history();
void add_history_command(const char* cmd);
void execute_history(char** args, int arg_count);
int get_history_count();
const char* get_history_entry(int index);

#endif
