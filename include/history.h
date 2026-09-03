#ifndef HISTORY_H
#define HISTORY_H

void inithistory();
void addhistorycmd(const char* cmd);
void executehistory(char** args, int argc);
int gethistorycount();
const char* gethistoryentry(int index);

#endif
