// shell prompt - shows <user@host:path>

#include "prompt.h"
#include <iostream>
#include <unistd.h>
#include <pwd.h>
#include <cstring>
#include <climits>

#ifndef host_name_max
#define host_name_max 256
#endif

// starting directory of the shell (shown as ~)
char shell_home[PATH_MAX];

// save the directory where shell was started
void initshellhome() {
    if (getcwd(shell_home, sizeof(shell_home)) == nullptr) {
        shell_home[0] = '\0';
    }
}

void displayprompt() {
    // get username from uid
    struct passwd* pw = getpwuid(geteuid());
    const char* username = pw ? pw->pw_name : "user";

    // get system hostname
    char hostname[host_name_max];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strncpy(hostname, "system", sizeof(hostname));
    }

    // get current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        strncpy(cwd, "?", sizeof(cwd));
    }

    std::cout << "<" << username << "@" << hostname << ":";

    // if inside shell home, replace path with ~
    size_t homelen = strlen(shell_home);
    if (homelen > 0 && strncmp(cwd, shell_home, homelen) == 0 &&
        (cwd[homelen] == '\0' || cwd[homelen] == '/')) {
        std::cout << "~" << (cwd + homelen);
    } else {
        std::cout << cwd;
    }

    std::cout << "> ";
    std::cout.flush();
}
