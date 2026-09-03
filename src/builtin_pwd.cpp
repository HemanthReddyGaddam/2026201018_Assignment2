// pwd builtin - print current directory

#include "builtin_pwd.h"
#include <iostream>
#include <unistd.h>
#include <climits>

void executepwd() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << cwd << "\n";
    } else {
        perror("pwd error");
    }
}
