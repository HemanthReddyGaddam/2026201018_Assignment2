// cd builtin - change directory

#include "builtin_cd.h"
#include "prompt.h"
#include<iostream>
#include<unistd.h>
#include<cstring>
#include<climits>

// stores previous dir for cd -
char prev_dir[PATH_MAX] = "";

void executecd(char** args, int argc) {
    // cd can only take one argument
    if (argc > 2) {
        std::cout << "Invalid arguments\n";
        return;
    }

    // save current dir before changing
    char curdir[PATH_MAX];
    if (getcwd(curdir, sizeof(curdir)) == nullptr) {
        perror("cd error getting cwd");
        return;
    }

    const char* target = nullptr;

    if (argc == 1 || strcmp(args[1], "~") == 0) {
        // no arg or cd ~ goes to shell home
        target = shell_home;
    } else if (strcmp(args[1], "-") == 0) {
        // cd - goes back to previous dir
        if (strlen(prev_dir) == 0) {
            std::cout << "cd: OLDPWD not set\n";
            return;
        }
        target = prev_dir;
        std::cout << prev_dir << "\n";
    } else if (strcmp(args[1], "..") == 0) {
        target = "..";
    } else if (strcmp(args[1], ".") == 0) {
        target = ".";
    } else {
        target = args[1];
    }

    if (chdir(target) == 0) {
        strncpy(prev_dir, curdir, sizeof(prev_dir));
    } else {
        perror("cd error");
    }
}
