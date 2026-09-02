#ifndef REDIRECTION_H
#define REDIRECTION_H

bool handle_redirection(char** args, int& arg_count, int& saved_stdin, int& saved_stdout);
void restore_redirection(int saved_stdin, int saved_stdout);

#endif