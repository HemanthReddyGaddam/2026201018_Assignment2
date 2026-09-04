#ifndef REDIRECTION_H
#define REDIRECTION_H

// Swaps standard file descriptors around for <, >, and >>, then restores them when done
bool handleredirection(char** args, int& argc, int& savedin, int& savedout);
void restoreredirection(int savedin, int savedout);

#endif