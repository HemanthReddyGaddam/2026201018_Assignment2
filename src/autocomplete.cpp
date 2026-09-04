// tab autocomplete and arrow key history browsing

#include "autocomplete.h"
#include "prompt.h"
#include "history.h"
#include<iostream>
#include<cstring>
#include<unistd.h>
#include<dirent.h>
#include<termios.h>
#include<climits>
#include<sys/select.h>

#define max_matches 512
#define max_name 256

// commands built into our shell
static const char* builtins[] = {
    "cd", "echo", "pwd", "ls", "pinfo", "search", "history", "exit",
    "activities", "ping", "fg", "bg", nullptr
};

static struct termios oldterm;
static bool termset = false;

// turn off canonical mode so we can read keys one at a time
static void enableraw() {
    tcgetattr(STDIN_FILENO, &oldterm);
    termset = true;

    struct termios raw = oldterm;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// put terminal back to normal
static void restoreterm() {
    if (termset) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldterm);
        termset = false;
    }
}

// find where the current word starts in the buffer
static int wordstart(const char* buf, int pos) {
    int i = pos - 1;
    while (i >= 0 && buf[i] != ' ' && buf[i] != '\t') {
        i--;
    }
    return i + 1;
}

// true if cursor is on the first word (command name)
static bool iscmdword(const char* buf, int start) {
    for (int i = 0; i < start; i++) {
        if (buf[i] != ' ' && buf[i] != '\t') {
            return false;
        }
    }
    return true;
}

// check if name is already in match list
static bool inlist(const char* name, char** list, int count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(list[i], name) == 0) {
            return true;
        }
    }
    return false;
}

// add a match if not already there
static void addmatch(const char* name, char** list, int* count) {
    if (*count >= max_matches || strlen(name) == 0) {
        return;
    }
    if (inlist(name, list, *count)) {
        return;
    }
    list[*count] = new char[strlen(name) + 1];
    strcpy(list[*count], name);
    (*count)++;
}

static void freematches(char** list, int count) {
    for (int i = 0; i < count; i++) {
        delete[] list[i];
    }
}

// match against builtin command names
static void addbuiltins(const char* prefix, char** list, int* count) {
    for (int i = 0; builtins[i] != nullptr; i++) {
        if (strncmp(builtins[i], prefix, strlen(prefix)) == 0) {
            addmatch(builtins[i], list, count);
        }
    }
}

// match against executables in PATH
static void addpathcmds(const char* prefix, char** list, int* count) {
    const char* path = getenv("PATH");
    if (path == nullptr) {
        return;
    }

    char pathcopy[PATH_MAX * 16];
    strncpy(pathcopy, path, sizeof(pathcopy) - 1);
    pathcopy[sizeof(pathcopy) - 1] = '\0';

    char* saveptr = nullptr;
    char* dir = strtok_r(pathcopy, ":", &saveptr);
    while (dir != nullptr) {
        DIR* dp = opendir(dir);
        if (dp != nullptr) {
            struct dirent* entry;
            while ((entry = readdir(dp)) != nullptr) {
                if (strncmp(entry->d_name, prefix, strlen(prefix)) != 0) {
                    continue;
                }
                char fullpath[PATH_MAX];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry->d_name);
                if (access(fullpath, X_OK) == 0) {
                    addmatch(entry->d_name, list, count);
                }
            }
            closedir(dp);
        }
        dir = strtok_r(nullptr, ":", &saveptr);
    }
}

// match files/folders in current directory
static void addfiles(const char* prefix, char** list, int* count) {
    DIR* dp = opendir(".");
    if (dp == nullptr) {
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dp)) != nullptr) {
        if (strncmp(entry->d_name, prefix, strlen(prefix)) == 0) {
            addmatch(entry->d_name, list, count);
        }
    }
    closedir(dp);
}

// find how many chars all matches share at the start
static int commonlen(char** list, int count) {
    if (count == 0) {
        return 0;
    }

    int len = strlen(list[0]);
    for (int i = 1; i < count; i++) {
        int j = 0;
        while (j < len && list[i][j] != '\0' && list[0][j] == list[i][j]) {
            j++;
        }
        len = j;
    }
    return len;
}

// redraw prompt + current input line
static void redraw(const char* buf) {
    std::cout << "\r";
    displayprompt();
    std::cout << buf << "\033[K";
    std::cout.flush();
}

// put a history command on the input line
static void setfromhistory(char* buf, int* pos, int maxlen, int index) {
    const char* entry = gethistoryentry(index);
    strncpy(buf, entry, maxlen - 1);
    buf[maxlen - 1] = '\0';
    *pos = strlen(buf);
    redraw(buf);
}

// up arrow - go to older command
static void historyup(char* buf, int* pos, int maxlen, int* histidx) {
    int count = gethistorycount();
    if (count == 0) {
        return;
    }

    if (*histidx == -1) {
        *histidx = count - 1;
    } else if (*histidx > 0) {
        (*histidx)--;
    }

    setfromhistory(buf, pos, maxlen, *histidx);
}

// down arrow - go to newer command
static void historydown(char* buf, int* pos, int maxlen, int* histidx) {
    if (*histidx == -1) {
        return;
    }

    int count = gethistorycount();
    if (*histidx < count - 1) {
        (*histidx)++;
        setfromhistory(buf, pos, maxlen, *histidx);
    }
}

// handle arrow key escape sequences
static void handlearrow(char* buf, int* pos, int maxlen, int* histidx, bool* tabshown) {
    char seq[16];
    int seqlen = 0;

    if (read(STDIN_FILENO, seq, 1) != 1) {
        return;
    }
    seqlen = 1;

    // read rest of escape sequence (like [A for up arrow)
    while (seqlen < (int)sizeof(seq) - 1) {
        fd_set fds;
        struct timeval tv = {0, 50000};
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0) {
            break;
        }
        if (read(STDIN_FILENO, seq + seqlen, 1) != 1) {
            break;
        }
        seqlen++;
        char last = seq[seqlen - 1];
        if (last == 'A' || last == 'B' || last == 'C' || last == 'D') {
            break;
        }
    }

    char arrow = 0;
    if (seq[0] == 'O' && seqlen >= 2) {
        arrow = seq[1];
    } else if (seq[0] == '[') {
        for (int i = 1; i < seqlen; i++) {
            if (seq[i] == 'A' || seq[i] == 'B' || seq[i] == 'C' || seq[i] == 'D') {
                arrow = seq[i];
                break;
            }
        }
    }

    if (arrow == 'A') {
        historyup(buf, pos, maxlen, histidx);
        *tabshown = false;
    } else if (arrow == 'B') {
        historydown(buf, pos, maxlen, histidx);
        *tabshown = false;
    }
}

// tab key - autocomplete command or filename
static void handletab(char* buf, int* pos, int maxlen, bool* tabshown) {
    int start = wordstart(buf, *pos);
    char prefix[max_name];
    int prefixlen = *pos - start;
    if (prefixlen >= max_name) {
        prefixlen = max_name - 1;
    }
    strncpy(prefix, buf + start, prefixlen);
    prefix[prefixlen] = '\0';

    char* matches[max_matches];
    int matchcount = 0;

    if (iscmdword(buf, start)) {
        // completing a command name
        addbuiltins(prefix, matches, &matchcount);
        addpathcmds(prefix, matches, &matchcount);
    } else {
        // completing a file/folder name
        addfiles(prefix, matches, &matchcount);
    }

    if (matchcount == 0) {
        freematches(matches, matchcount);
        return;
    }

    if (matchcount == 1) {
        // only one match - fill it in
        const char* fill = matches[0];
        int filllen = strlen(fill);
        for (int i = prefixlen; i < filllen && *pos < maxlen - 1; i++) {
            buf[*pos] = fill[i];
            (*pos)++;
            std::cout << fill[i];
        }
        buf[*pos] = '\0';
        std::cout.flush();
        *tabshown = false;
    } else {
        int shared = commonlen(matches, matchcount);
        if (shared > prefixlen) {
            // multiple matches but same prefix - fill common part
            for (int i = prefixlen; i < shared && *pos < maxlen - 1; i++) {
                buf[*pos] = matches[0][i];
                (*pos)++;
                std::cout << matches[0][i];
            }
            buf[*pos] = '\0';
            std::cout.flush();
            *tabshown = false;
        } else if (!*tabshown) {
            // show all matches on screen
            std::cout << '\n';
            for (int i = 0; i < matchcount; i++) {
                std::cout << matches[i];
                if (i + 1 < matchcount) {
                    std::cout << ' ';
                }
            }
            std::cout << '\n';
            displayprompt();
            std::cout << buf;
            std::cout.flush();
            *tabshown = true;
        }
    }

    freematches(matches, matchcount);
}

// main input reading function - handles tab, arrows, backspace, ctrl-d
bool readinput(char* buffer, int maxlen) {
    if (maxlen <= 0) {
        return false;
    }

    // if input is piped in (not a real terminal), just use getline
    if (!isatty(STDIN_FILENO)) {
        if (std::cin.getline(buffer, maxlen)) {
            return true;
        }
        return false;
    }

    enableraw();

    int pos = 0;
    buffer[0] = '\0';
    bool tabshown = false;
    int histidx = -1;

    while (true) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {
            restoreterm();
            return false;
        }

        if (c == '\n' || c == '\r') {
            buffer[pos] = '\0';
            std::cout << "\n";
            restoreterm();
            return true;
        }

        if (c == 4) {  // ctrl-d
            if (pos == 0) {
                restoreterm();
                return false;  // logout
            }
            continue;
        }

        if (c == 27) {  // escape - probably arrow key
            handlearrow(buffer, &pos, maxlen, &histidx, &tabshown);
            continue;
        }

        if (c == '\t') {
            handletab(buffer, &pos, maxlen, &tabshown);
            histidx = -1;
            continue;
        }

        if (c == 127 || c == 8) {  // backspace
            if (pos > 0) {
                pos--;
                buffer[pos] = '\0';
                std::cout << "\b \b";
                std::cout.flush();
            }
            tabshown = false;
            histidx = -1;
            continue;
        }

        // normal printable character
        if (c >= 32 && pos < maxlen - 1) {
            buffer[pos] = c;
            pos++;
            buffer[pos] = '\0';
            std::cout << c;
            std::cout.flush();
            tabshown = false;
            histidx = -1;
        }
    }
}
