#ifndef HISTORY_H
#define HISTORY_H

// keeps track of past commands so users can spam the up arrow or run history
void inithistory();
void addhistorycmd(const char* cmd);
// Runs the command history
void executehistory(char** args, int argc);
int gethistorycount();
const char* gethistoryentry(int index);

#endif