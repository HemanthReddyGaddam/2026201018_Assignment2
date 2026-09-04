// input/output redirection - < > >>

#include "redirection.h"
#include<iostream>
#include<fcntl.h>
#include<unistd.h>
#include<cstring>

bool handleredirection(char** args, int& argc, int& savedin, int& savedout) {
    savedin = -1;
    savedout = -1;

    char* infile = nullptr;
    char* outfile = nullptr;
    bool append = false;

    int newargc = 0;
    char* newargs[128];

    // go through args and pull out < > >> operators
    for (int i = 0; i < argc; i++) {
        if (strcmp(args[i], "<") == 0) {
            if (i + 1 < argc) {
                infile = args[++i];
            } else {
                std::cout << "Syntax error near unexpected token '<'\n";
                return false;
            }
        } else if (strcmp(args[i], ">") == 0) {
            if (i + 1 < argc) {
                outfile = args[++i];
                append = false;
            } else {
                std::cout << "Syntax error near unexpected token '>'\n";
                return false;
            }
        } else if (strcmp(args[i], ">>") == 0) {
            if (i + 1 < argc) {
                outfile = args[++i];
                append = true;
            } else {
                std::cout << "Syntax error near unexpected token '>>'\n";
                return false;
            }
        } else {
            newargs[newargc++] = args[i];
        }
    }

    newargs[newargc] = nullptr;

    // redirect stdin from file
    if (infile != nullptr) {
        int fd = open(infile, O_RDONLY);
        if (fd < 0) {
            perror("Input file error");
            return false;
        }
        savedin = dup(STDIN_FILENO);
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    // redirect stdout to file
    if (outfile != nullptr) {
        int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
        int fd = open(outfile, flags, 0644);
        if (fd < 0) {
            perror("Output file error");
            if (savedin != -1) {
                restoreredirection(savedin, -1);
            }
            return false;
        }
        savedout = dup(STDOUT_FILENO);
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    // put cleaned args back (without < > >> parts)
    for (int i = 0; i < newargc; i++) {
        args[i] = newargs[i];
    }
    args[newargc] = nullptr;
    argc = newargc;

    return true;
}

// put stdin/stdout back to normal after command runs
void restoreredirection(int savedin, int savedout) {
    if (savedin != -1) {
        dup2(savedin, STDIN_FILENO);
        close(savedin);
    }
    if (savedout != -1) {
        dup2(savedout, STDOUT_FILENO);
        close(savedout);
    }
}
