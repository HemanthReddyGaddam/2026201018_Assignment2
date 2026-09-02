#include "builtin_cd.h"
#include "prompt.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <climits>

char PREV_DIR[PATH_MAX] = "";

void execute_cd(char** args, int arg_count) {
    // Check constraint: More than 1 argument (excluding "cd" itself) is an error
    if (arg_count > 2) {
        std::cout << "Invalid arguments\n";
        return;
    }

    char current_dir[PATH_MAX];
    if (getcwd(current_dir, sizeof(current_dir)) == nullptr) {
        perror("cd error getting cwd");
        return;
    }

    const char* target_dir = nullptr;

    if (arg_count == 1 || strcmp(args[1], "~") == 0) {
        // Default cd or cd ~ moves to shell home directory
        target_dir = SHELL_HOME;
    } else if (strcmp(args[1], "-") == 0) {
        // cd - moves to previous working directory
        if (strlen(PREV_DIR) == 0) {
            std::cout << "cd: OLDPWD not set\n";
            return;
        }
        target_dir = PREV_DIR;
        std::cout << PREV_DIR << "\n";
    } else if (strcmp(args[1], "..") == 0) {
        // cd .. moves up one directory level
        target_dir = "..";
    } else if (strcmp(args[1], ".") == 0) {
        // cd . remains in the current directory
        target_dir = ".";
    } else {
        target_dir = args[1];
    }

    // Attempt directory change using chdir system call
    if (chdir(target_dir) == 0) {
        // Save previous working directory on success
        strncpy(PREV_DIR, current_dir, sizeof(PREV_DIR));
    } else {
        perror("cd error");
    }
}