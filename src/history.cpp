#include "history.h"
#include "prompt.h"
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#define MAX_HISTORY_STORED 20
#define MAX_HISTORY_DISPLAY 10
#define MAX_CMD_LEN 1024
#define HISTORY_FILE ".shell_history"

static char history[MAX_HISTORY_STORED][MAX_CMD_LEN];
static int history_count = 0;

static char history_file_path[PATH_MAX];

static void build_history_path() {
    snprintf(history_file_path, sizeof(history_file_path), "%s/%s", SHELL_HOME, HISTORY_FILE);
}

static void persist_history() {
    FILE* fp = fopen(history_file_path, "w");
    if (fp == nullptr) {
        return;
    }
    for (int i = 0; i < history_count; i++) {
        fprintf(fp, "%s\n", history[i]);
    }
    fclose(fp);
}

void init_history() {
    build_history_path();
    history_count = 0;

    FILE* fp = fopen(history_file_path, "r");
    if (fp == nullptr) {
        return;
    }

    char line[MAX_CMD_LEN];
    char loaded[MAX_HISTORY_STORED][MAX_CMD_LEN];
    int loaded_count = 0;

    while (fgets(line, sizeof(line), fp) != nullptr) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        if (line[0] == '\0') {
            continue;
        }
        if (loaded_count < MAX_HISTORY_STORED) {
            strncpy(loaded[loaded_count], line, MAX_CMD_LEN - 1);
            loaded[loaded_count][MAX_CMD_LEN - 1] = '\0';
            loaded_count++;
        } else {
            for (int i = 0; i < MAX_HISTORY_STORED - 1; i++) {
                strcpy(loaded[i], loaded[i + 1]);
            }
            strncpy(loaded[MAX_HISTORY_STORED - 1], line, MAX_CMD_LEN - 1);
            loaded[MAX_HISTORY_STORED - 1][MAX_CMD_LEN - 1] = '\0';
        }
    }
    fclose(fp);

    for (int i = 0; i < loaded_count; i++) {
        strcpy(history[i], loaded[i]);
    }
    history_count = loaded_count;
}

void add_history_command(const char* cmd) {
    if (cmd == nullptr) {
        return;
    }

    char trimmed[MAX_CMD_LEN];
    strncpy(trimmed, cmd, MAX_CMD_LEN - 1);
    trimmed[MAX_CMD_LEN - 1] = '\0';

    size_t len = strlen(trimmed);
    while (len > 0 && (trimmed[len - 1] == ' ' || trimmed[len - 1] == '\t')) {
        trimmed[len - 1] = '\0';
        len--;
    }

    size_t start = 0;
    while (trimmed[start] == ' ' || trimmed[start] == '\t') {
        start++;
    }
    if (trimmed[start] == '\0') {
        return;
    }

    if (start > 0) {
        strncpy(trimmed, trimmed + start, MAX_CMD_LEN - start);
        trimmed[MAX_CMD_LEN - 1] = '\0';
    }

    if (history_count < MAX_HISTORY_STORED) {
        strncpy(history[history_count], trimmed, MAX_CMD_LEN - 1);
        history[history_count][MAX_CMD_LEN - 1] = '\0';
        history_count++;
    } else {
        for (int i = 0; i < MAX_HISTORY_STORED - 1; i++) {
            strcpy(history[i], history[i + 1]);
        }
        strncpy(history[MAX_HISTORY_STORED - 1], trimmed, MAX_CMD_LEN - 1);
        history[MAX_HISTORY_STORED - 1][MAX_CMD_LEN - 1] = '\0';
    }

    persist_history();
}

void execute_history(char** args, int arg_count) {
    int display_count = MAX_HISTORY_DISPLAY;
    if (arg_count >= 2) {
        display_count = atoi(args[1]);
        if (display_count < 0) {
            display_count = 0;
        }
    }

    int start = history_count - display_count;
    if (start < 0) {
        start = 0;
    }

    for (int i = start; i < history_count; i++) {
        std::cout << history[i] << "\n";
    }
}

int get_history_count() {
    return history_count;
}

const char* get_history_entry(int index) {
    if (index < 0 || index >= history_count) {
        return "";
    }
    return history[index];
}
