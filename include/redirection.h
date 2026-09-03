#ifndef REDIRECTION_H
#define REDIRECTION_H

bool handleredirection(char** args, int& argc, int& savedin, int& savedout);
void restoreredirection(int savedin, int savedout);

#endif
