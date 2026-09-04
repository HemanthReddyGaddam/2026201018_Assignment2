// pipe commands together with |

#include "pipeline.h"
#include "redirection.h"
#include "executor.h"
#include "signals.h"
#include "builtin_cd.h"
#include "builtin_echo.h"
#include "builtin_pwd.h"
#include "builtin_ls.h"
#include "builtin_pinfo.h"
#include "builtin_search.h"
#include "parser.h"
#include<iostream>
#include<unistd.h>
#include<sys/wait.h>
#include<cstring>
#include<cstdlib>

#define max_cmds 64
#define max_pipes 63

void executepipeline(char* cmdline) {
    char* cmds[max_cmds];
    int cmdcount = 0;

    // split the line by |
    char* saveptr = nullptr;
    char* part = strtok_r(cmdline, "|", &saveptr);
    while (part != nullptr && cmdcount < max_cmds) {
        cmds[cmdcount++] = part;
        part = strtok_r(nullptr, "|", &saveptr);
    }

    if (cmdcount == 0) {
        return;
    }

    int pipecount = cmdcount - 1;
    int pipefds[2 * max_pipes];
    pid_t childpids[max_cmds];
    pid_t pgid = 0;

    // create all pipes first
    for (int i = 0; i < pipecount; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("pipe error");
            return;
        }
    }

    // fork a child for each command in the pipe
    for (int i = 0; i < cmdcount; i++) {
        char* args[128];
        int argc = 0;

        char* argptr = nullptr;
        char* token = strtok_r(cmds[i], " \t\n", &argptr);
        while (token != nullptr && argc < 127) {
            stripquotes(token);
            args[argc++] = token;
            token = strtok_r(nullptr, " \t\n", &argptr);
        }
        args[argc] = nullptr;

        if (argc == 0) {
            childpids[i] = -1;
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork error");
            return;
        }

        if (pid == 0) {
            // child setup
            if (i == 0) {
                setpgid(0, 0);
            } else {
                setpgid(0, pgid);
            }

            // connect stdin to previous pipe (if not first cmd)
            if (i != 0) {
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            }
            // connect stdout to next pipe (if not last cmd)
            if (i != cmdcount - 1) {
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
            }

            // close all pipe fds in child
            for (int j = 0; j < 2 * pipecount; j++) {
                close(pipefds[j]);
            }

            // handle< > >> inside pipeline too
            int savedin = -1, savedout = -1;
            if (!handleredirection(args, argc, savedin, savedout)) {
                _exit(EXIT_FAILURE);
            }

            // run builtin or external command
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
            } else if (execvp(args[0], args) < 0) {
                std::cout << "ERROR: '" << args[0] << "' is not a valid command\n";
                _exit(EXIT_FAILURE);
            }

            restoreredirection(savedin, savedout);
            fflush(stdout);  // needed so piped output actually goes through
            _exit(EXIT_SUCCESS);
        }

        childpids[i] = pid;
        if (i == 0) {
            pgid = pid;
        } else {
            setpgid(pid, pgid);
        }
    }

    // parent closes its copy of pipe fds
    for (int i = 0; i < 2 * pipecount; i++) {
        close(pipefds[i]);
    }

    // wait for all children
    fg_pid = pgid;
    for (int i = 0; i < cmdcount; i++) {
        if (childpids[i] > 0) {
            int status;
            waitpid(childpids[i], &status, WUNTRACED);
            if (WIFSTOPPED(status)) {
                addjob(childpids[i], cmds[i]);
                updatejobstatus(childpids[i], false);
            }
        }
    }
    fg_pid = 0;
}
