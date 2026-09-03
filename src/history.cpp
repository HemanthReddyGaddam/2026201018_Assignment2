// command history - stores last 20 cmds in .shell_history file

#include "history.h"
#include "prompt.h"
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#define max_stored 20   // max commands to remember
#define max_show 10     // default number history prints
#define max_cmd 1024
#define histfile ".shell_history"

static char histbuf[max_stored][max_cmd];
static int histcount = 0;
static char histfilepath[PATH_MAX];

// path to history file inside shell home
static void buildhistorypath() {
    snprintf(histfilepath, sizeof(histfilepath), "%s/%s", shell_home, histfile);
}

// write all commands to file
static void savehistory() {
    FILE* fp = fopen(histfilepath, "w");
    if (fp == nullptr) {
        return;
    }
    for (int i = 0; i < histcount; i++) {
        fprintf(fp, "%s\n", histbuf[i]);
    }
    fclose(fp);
}

// load history from file when shell starts
void inithistory() {
    buildhistorypath();
    histcount = 0;

    FILE* fp = fopen(histfilepath, "r");
    if (fp == nullptr) {
        return;  // no history file yet, that's fine
    }

    char line[max_cmd];
    char loaded[max_stored][max_cmd];
    int loadedcount = 0;

    while (fgets(line, sizeof(line), fp) != nullptr) {
        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        if (line[0] == '\0') {
            continue;
        }

        if (loadedcount < max_stored) {
            strncpy(loaded[loadedcount], line, max_cmd - 1);
            loaded[loadedcount][max_cmd - 1] = '\0';
            loadedcount++;
        } else {
            // drop oldest entry when full
            for (int i = 0; i < max_stored - 1; i++) {
                strcpy(loaded[i], loaded[i + 1]);
            }
            strncpy(loaded[max_stored - 1], line, max_cmd - 1);
            loaded[max_stored - 1][max_cmd - 1] = '\0';
        }
    }
    fclose(fp);

    for (int i = 0; i < loadedcount; i++) {
        strcpy(histbuf[i], loaded[i]);
    }
    histcount = loadedcount;
}

// add a new command to history
void addhistorycmd(const char* cmd) {
    if (cmd == nullptr) {
        return;
    }

    // trim spaces from ends
    char trimmed[max_cmd];
    strncpy(trimmed, cmd, max_cmd - 1);
    trimmed[max_cmd - 1] = '\0';

    int len = strlen(trimmed);
    while (len > 0 && (trimmed[len - 1] == ' ' || trimmed[len - 1] == '\t')) {
        trimmed[len - 1] = '\0';
        len--;
    }

    int start = 0;
    while (trimmed[start] == ' ' || trimmed[start] == '\t') {
        start++;
    }
    if (trimmed[start] == '\0') {
        return;  // empty line, skip
    }

    if (start > 0) {
        strncpy(trimmed, trimmed + start, max_cmd - start);
        trimmed[max_cmd - 1] = '\0';
    }

    if (histcount < max_stored) {
        strncpy(histbuf[histcount], trimmed, max_cmd - 1);
        histbuf[histcount][max_cmd - 1] = '\0';
        histcount++;
    } else {
        // shift old ones out
        for (int i = 0; i < max_stored - 1; i++) {
            strcpy(histbuf[i], histbuf[i + 1]);
        }
        strncpy(histbuf[max_stored - 1], trimmed, max_cmd - 1);
        histbuf[max_stored - 1][max_cmd - 1] = '\0';
    }

    savehistory();
}

// history command - print recent commands
void executehistory(char** args, int argc) {
    int showcount = max_show;
    if (argc >= 2) {
        showcount = atoi(args[1]);
        if (showcount < 0) {
            showcount = 0;
        }
    }

    int start = histcount - showcount;
    if (start < 0) {
        start = 0;
    }

    for (int i = start; i < histcount; i++) {
        std::cout << histbuf[i] << "\n";
    }
}

int gethistorycount() {
    return histcount;
}

const char* gethistoryentry(int index) {
    if (index < 0 || index >= histcount) {
        return "";
    }
    return histbuf[index];
}
