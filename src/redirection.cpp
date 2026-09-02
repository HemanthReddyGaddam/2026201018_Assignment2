#include "redirection.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

bool handle_redirection(char** args, int& arg_count, int& saved_stdin, int& saved_stdout) {
    saved_stdin = -1;
    saved_stdout = -1;

    char* input_file = nullptr;
    char* output_file = nullptr;
    bool append_mode = false;

    int new_arg_count = 0;
    char* clean_args[128];

    for (int i = 0; i < arg_count; ++i) {
        if (strcmp(args[i], "<") == 0) {
            if (i + 1 < arg_count) {
                input_file = args[++i];
            } else {
                std::cout << "Syntax error near unexpected token '<'\n";
                return false;
            }
        } else if (strcmp(args[i], ">") == 0) {
            if (i + 1 < arg_count) {
                output_file = args[++i];
                append_mode = false;
            } else {
                std::cout << "Syntax error near unexpected token '>'\n";
                return false;
            }
        } else if (strcmp(args[i], ">>") == 0) {
            if (i + 1 < arg_count) {
                output_file = args[++i];
                append_mode = true;
            } else {
                std::cout << "Syntax error near unexpected token '>>'\n";
                return false;
            }
        } else {
            clean_args[new_arg_count++] = args[i];
        }
    }

    clean_args[new_arg_count] = nullptr;

    // Handle input redirection (<)
    if (input_file != nullptr) {
        int fd_in = open(input_file, O_RDONLY);
        if (fd_in < 0) {
            perror("Input file error");
            return false;
        }
        saved_stdin = dup(STDIN_FILENO);
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }

    // Handle output redirection (> or >>)
    if (output_file != nullptr) {
        int flags = O_WRONLY | O_CREAT | (append_mode ? O_APPEND : O_TRUNC);
        // Create with permissions 0644 (rw-r--r--)
        int fd_out = open(output_file, flags, 0644);
        if (fd_out < 0) {
            perror("Output file error");
            if (saved_stdin != -1) restore_redirection(saved_stdin, -1);
            return false;
        }
        saved_stdout = dup(STDOUT_FILENO);
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    // Update original args to strip redirection operator components
    for (int i = 0; i < new_arg_count; ++i) {
        args[i] = clean_args[i];
    }
    args[new_arg_count] = nullptr;
    arg_count = new_arg_count;

    return true;
}

void restore_redirection(int saved_stdin, int saved_stdout) {
    if (saved_stdin != -1) {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
    }
    if (saved_stdout != -1) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
}