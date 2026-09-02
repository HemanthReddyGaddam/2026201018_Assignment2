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
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <cstdlib>

void execute_pipeline(char* command_str) {
    char* commands[64];
    int cmd_count = 0;

    char* saveptr_pipe = nullptr;
    char* token = strtok_r(command_str, "|", &saveptr_pipe);
    while (token != nullptr && cmd_count < 64) {
        commands[cmd_count++] = token;
        token = strtok_r(nullptr, "|", &saveptr_pipe);
    }

    if (cmd_count == 0) {
        return;
    }

    int num_pipes = cmd_count - 1;
    int* pipefds = nullptr;

    if (num_pipes > 0) {
        pipefds = new int[2 * num_pipes];
        for (int i = 0; i < num_pipes; ++i) {
            if (pipe(pipefds + i * 2) < 0) {
                perror("pipe error");
                delete[] pipefds;
                return;
            }
        }
    }

    pid_t* child_pids = new pid_t[cmd_count];
    pid_t pgid = 0;

    for (int i = 0; i < cmd_count; ++i) {
        char* args[128];
        int arg_count = 0;

        char* saveptr_arg = nullptr;
        char* arg_token = strtok_r(commands[i], " \t\n", &saveptr_arg);
        while (arg_token != nullptr && arg_count < 127) {
            args[arg_count++] = arg_token;
            arg_token = strtok_r(nullptr, " \t\n", &saveptr_arg);
        }
        args[arg_count] = nullptr;

        if (arg_count == 0) {
            child_pids[i] = -1;
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork error");
            delete[] pipefds;
            delete[] child_pids;
            return;
        }

        if (pid == 0) {
            if (i == 0) {
                setpgid(0, 0);
            } else {
                setpgid(0, pgid);
            }

            if (i != 0) {
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            }

            if (i != cmd_count - 1) {
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
            }

            if (num_pipes > 0) {
                for (int j = 0; j < 2 * num_pipes; ++j) {
                    close(pipefds[j]);
                }
            }

            int saved_in = -1, saved_out = -1;
            if (!handle_redirection(args, arg_count, saved_in, saved_out)) {
                _exit(EXIT_FAILURE);
            }

            if (strcmp(args[0], "pwd") == 0) {
                execute_pwd();
            } else if (strcmp(args[0], "echo") == 0) {
                execute_echo(args, arg_count);
            } else if (strcmp(args[0], "cd") == 0) {
                execute_cd(args, arg_count);
            } else if (strcmp(args[0], "ls") == 0) {
                execute_ls(args, arg_count);
            } else if (strcmp(args[0], "pinfo") == 0) {
                execute_pinfo(args, arg_count);
            } else if (strcmp(args[0], "search") == 0) {
                execute_search(args, arg_count);
            } else if (execvp(args[0], args) < 0) {
                std::cout << "ERROR: '" << args[0] << "' is not a valid command\n";
                _exit(EXIT_FAILURE);
            }

            restore_redirection(saved_in, saved_out);
            fflush(stdout);
            _exit(EXIT_SUCCESS);
        }

        child_pids[i] = pid;
        if (i == 0) {
            pgid = pid;
        } else {
            setpgid(pid, pgid);
        }
    }

    if (num_pipes > 0) {
        for (int i = 0; i < 2 * num_pipes; ++i) {
            close(pipefds[i]);
        }
    }

    fg_pid = pgid;

    for (int i = 0; i < cmd_count; ++i) {
        if (child_pids[i] > 0) {
            int status;
            waitpid(child_pids[i], &status, WUNTRACED);
            if (WIFSTOPPED(status)) {
                add_job(child_pids[i], commands[i]);
                update_job_status(child_pids[i], false);
            }
        }
    }

    fg_pid = 0;

    delete[] pipefds;
    delete[] child_pids;
}
