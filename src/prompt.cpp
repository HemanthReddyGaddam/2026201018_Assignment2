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
    //get the current working directory and store it in SHELL_HOME
    if (getcwd(SHELL_HOME, sizeof(SHELL_HOME)) == nullptr) {
        SHELL_HOME[0] = '\0';
    }
}

void displayprompt() {
    // Dynamically get the username
    // geteuid() system call gets the active user's ID; getpwuid() function converts that ID to a user record.
    struct passwd* pw = getpwuid(geteuid());
    const char* username = pw ? pw->pw_name : "user";

    // Dynamically get the hostname
    // gethostname() system call gets the hostname of the system and stores it in the hostname variable
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strncpy(hostname, "system", sizeof(hostname));
    }

    // Retrieve current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        strncpy(cwd, "?", sizeof(cwd));
    }

    // Display the username, hostname, and the current working directory
    std::cout << "<" << username << "@" << hostname << ":";

    // Checkinf  if the current dir is inside or matches our static home dir.
    size_t home_len = strlen(SHELL_HOME);
    if (home_len > 0 && strncmp(cwd, SHELL_HOME, home_len) == 0) {
        // Replace base shell directory with ~
        std::cout << "~" << (cwd + home_len);
    } else {
        std::cout << cwd;
    }

    std::cout << "> ";
    std::cout.flush();
}