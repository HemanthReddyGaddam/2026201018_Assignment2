// main shell loop - reads input and runs commands

#include "prompt.h"
#include "builtin_cd.h"
#include "builtin_echo.h"
#include "builtin_pwd.h"
#include "builtin_ls.h"
#include "executor.h"
#include "builtin_pinfo.h"
#include "builtin_search.h"
#include "redirection.h"
#include "pipeline.h"
#include "signals.h"
#include "autocomplete.h"
#include "history.h"
#include "parser.h"
#include <iostream>
#include <cstring>
#include <unistd.h>

#define max_input 1024

// check command name and call the right builtin or external program
static void runbuiltin(char** args, int argc, bool bg) {
    if (strcmp(args[0], "pwd") == 0) {
        executepwd();
    } else if (strcmp(args[0], "echo") == 0) {
        executeecho(args, argc);
    } else if (strcmp(args[0], "cd") == 0) {
        executecd(args, argc);
    } else if (strcmp(args[0], "ls") == 0) {
        executels(args, argc);
    } else if (strcmp(args[0], "pinfo") == 0) {
        executepinfo(args, argc);
    } else if (strcmp(args[0], "search") == 0) {
        executesearch(args, argc);
    } else if (strcmp(args[0], "history") == 0) {
        executehistory(args, argc);
    } else if (strcmp(args[0], "activities") == 0) {
        executeactivities();
    } else if (strcmp(args[0], "ping") == 0) {
        executeping(args, argc);
    } else if (strcmp(args[0], "fg") == 0) {
        executefg(args, argc);
    } else if (strcmp(args[0], "bg") == 0) {
        executebg(args, argc);
    } else if (strcmp(args[0], "exit") == 0) {
        _exit(0);
    } else {
        // not a builtin, so fork and exec
        executesystemcmd(args, argc, bg);
    }
}

// handle one full input line (can have multiple commands separated by ;)
void processcommandline(char* line) {
    char* saveptr = nullptr;
    // split by semicolon first
    char* cmd = strtok_r(line, ";", &saveptr);

    while (cmd != nullptr) {
        // pipe commands go to pipeline handler
        if (strchr(cmd, '|') != nullptr) {
            executepipeline(cmd);
        } else {
            char* args[128];
            int argc = 0;

            // tokenize on spaces and tabs
            char* argptr = nullptr;
            char* token = strtok_r(cmd, " \t\n", &argptr);
            while (token != nullptr && argc < 127) {
                stripquotes(token);  // remove "quotes" from args
                args[argc++] = token;
                token = strtok_r(nullptr, " \t\n", &argptr);
            }
            args[argc] = nullptr;

            if (argc > 0) {
                bool bg = false;
                // check for background & at the end
                if (strcmp(args[argc - 1], "&") == 0) {
                    bg = true;
                    args[argc - 1] = nullptr;
                    argc--;
                }

                if (argc > 0) {
                    int savedin = -1, savedout = -1;
                    // set up < > >> if present, then run command
                    if (handleredirection(args, argc, savedin, savedout)) {
                        runbuiltin(args, argc, bg);
                        restoreredirection(savedin, savedout);
                    }
                }
            }
        }
        cmd = strtok_r(nullptr, ";", &saveptr);
    }
}

int main() {
    // setup stuff before shell starts
    initshellhome();       // save starting directory as ~
    inithistory();         // load old commands from file
    initsignalhandlers();  // ctrl-c, ctrl-z, etc.

    char input[max_input];

    while (true) {
        displayprompt();

        // readinput returns false on ctrl-d (logout)
        if (!readinput(input, sizeof(input))) {
            std::cout << "\n";
            break;
        }

        addhistorycmd(input);
        processcommandline(input);
    }

    return 0;
}
