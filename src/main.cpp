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
#include <iostream>
#include <cstring>
#include <unistd.h>

#define MAX_INPUT 1024

void process_command_line(char* line) {
    char* saveptr_cmd;
    char* command = strtok_r(line, ";", &saveptr_cmd);

    while (command != nullptr) {
        if (strchr(command, '|') != nullptr) {
            execute_pipeline(command);
        } else {
            char* args[128];
            int arg_count = 0;

            char* saveptr_arg;
            char* token = strtok_r(command, " \t\n", &saveptr_arg);
            while (token != nullptr && arg_count < 127) {
                args[arg_count++] = token;
                token = strtok_r(nullptr, " \t\n", &saveptr_arg);
            }
            args[arg_count] = nullptr;

            if (arg_count > 0) {
                bool is_background = false;
                if (strcmp(args[arg_count - 1], "&") == 0) {
                    is_background = true;
                    args[arg_count - 1] = nullptr;
                    arg_count--;
                }

                if (arg_count > 0) {
                    int saved_stdin = -1, saved_stdout = -1;
                    if (handle_redirection(args, arg_count, saved_stdin, saved_stdout)) {
                        if (strcmp(args[0], "pwd") == 0) execute_pwd();
                        else if (strcmp(args[0], "echo") == 0) execute_echo(args, arg_count);
                        else if (strcmp(args[0], "cd") == 0) execute_cd(args, arg_count);
                        else if (strcmp(args[0], "ls") == 0) execute_ls(args, arg_count);
                        else if (strcmp(args[0], "pinfo") == 0) execute_pinfo(args, arg_count);
                        else if (strcmp(args[0], "search") == 0) execute_search(args, arg_count);
                        else if (strcmp(args[0], "history") == 0) execute_history(args, arg_count);
                        else if (strcmp(args[0], "activities") == 0) execute_activities();
                        else if (strcmp(args[0], "ping") == 0) execute_ping(args, arg_count);
                        else if (strcmp(args[0], "fg") == 0) execute_fg(args, arg_count);
                        else if (strcmp(args[0], "bg") == 0) execute_bg(args, arg_count);
                        else if (strcmp(args[0], "exit") == 0) _exit(0);
                        else execute_system_command(args, arg_count, is_background);

                        restore_redirection(saved_stdin, saved_stdout);
                    }
                }
            }
        }
        command = strtok_r(nullptr, ";", &saveptr_cmd);
    }
}

int main() {
    init_shell_home();
    init_history();
    init_signal_handlers();

    char input[MAX_INPUT];

    while (true) {
        display_prompt();

        if (!read_line_with_autocomplete(input, sizeof(input))) {
            std::cout << "\n";
            break;
        }

        add_history_command(input);
        process_command_line(input);
    }

    return 0;
}