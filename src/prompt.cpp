#include "prompt.h"
#include <iostream>
#include <unistd.h>
#include <pwd.h>
#include <cstring>
#include <climits>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

// Define the global variable here
char SHELL_HOME[PATH_MAX];

void init_shell_home() {
    if (getcwd(SHELL_HOME, sizeof(SHELL_HOME)) == nullptr) {
        SHELL_HOME[0] = '\0';
    }
}

void display_prompt() {
    // Dynamically retrieve username
    struct passwd* pw = getpwuid(geteuid());
    const char* username = pw ? pw->pw_name : "user";

    // Dynamically retrieve hostname
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strncpy(hostname, "system", sizeof(hostname));
    }

    // Retrieve current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        strncpy(cwd, "?", sizeof(cwd));
    }

    std::cout << "<" << username << "@" << hostname << ":";

    // Calculate relative path to SHELL_HOME
    size_t home_len = strlen(SHELL_HOME);
    if (home_len > 0 && strncmp(cwd, SHELL_HOME, home_len) == 0) {
        // Replace base shell directory with ~
        if (cwd[home_len] == '\0' || cwd[home_len] == '/') {
            std::cout << "~" << (cwd + home_len);
        } else {
            std::cout << cwd;
        }
    } else {
        std::cout << cwd;
    }

    std::cout << "> ";
    std::cout.flush();
}