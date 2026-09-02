#include "autocomplete.h"
#include "prompt.h"
#include "history.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <dirent.h>
#include <termios.h>
#include <climits>
#include <sys/select.h>

#define MAX_MATCHES 512
#define MAX_NAME_LEN 256

static const char* BUILTINS[] = {
    "cd", "echo", "pwd", "ls", "pinfo", "search", "history", "exit",
    "activities", "ping", "fg", "bg", nullptr
};

static struct termios orig_termios;
static bool termios_saved = false;

static void enable_raw_mode() {
    // Always snapshot current terminal state before entering raw mode.
    tcgetattr(STDIN_FILENO, &orig_termios);
    termios_saved = true;

    struct termios raw = orig_termios;
    // Disable canonical mode and echo — we print input ourselves.
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void restore_terminal() {
    if (termios_saved) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        termios_saved = false;
    }
}

static void write_char(char c) {
    std::cout << c;
    std::cout.flush();
}

static void write_str(const char* s) {
    std::cout << s;
    std::cout.flush();
}

static int find_word_start(const char* buf, int cursor) {
    int i = cursor - 1;
    while (i >= 0 && buf[i] != ' ' && buf[i] != '\t') {
        i--;
    }
    return i + 1;
}

static bool is_completing_command(const char* buf, int word_start) {
    for (int i = 0; i < word_start; i++) {
        if (buf[i] != ' ' && buf[i] != '\t') {
            return false;
        }
    }
    return true;
}

static bool match_in_list(const char* name, char** matches, int count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(matches[i], name) == 0) {
            return true;
        }
    }
    return false;
}

static void add_match(const char* name, char** matches, int* count) {
    if (*count >= MAX_MATCHES || strlen(name) == 0) {
        return;
    }
    if (match_in_list(name, matches, *count)) {
        return;
    }
    matches[*count] = new char[strlen(name) + 1];
    strcpy(matches[*count], name);
    (*count)++;
}

static void free_matches(char** matches, int count) {
    for (int i = 0; i < count; i++) {
        delete[] matches[i];
        matches[i] = nullptr;
    }
}

static void collect_builtin_matches(const char* prefix, char** matches, int* count) {
    for (int i = 0; BUILTINS[i] != nullptr; i++) {
        if (strncmp(BUILTINS[i], prefix, strlen(prefix)) == 0) {
            add_match(BUILTINS[i], matches, count);
        }
    }
}

static void collect_path_matches(const char* prefix, char** matches, int* count) {
    const char* path_env = getenv("PATH");
    if (path_env == nullptr) {
        return;
    }

    char path_copy[PATH_MAX * 16];
    strncpy(path_copy, path_env, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    char* saveptr = nullptr;
    char* dir = strtok_r(path_copy, ":", &saveptr);
    while (dir != nullptr) {
        DIR* dp = opendir(dir);
        if (dp != nullptr) {
            struct dirent* entry;
            while ((entry = readdir(dp)) != nullptr) {
                if (strncmp(entry->d_name, prefix, strlen(prefix)) != 0) {
                    continue;
                }
                char full_path[PATH_MAX];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir, entry->d_name);
                if (access(full_path, X_OK) == 0) {
                    add_match(entry->d_name, matches, count);
                }
            }
            closedir(dp);
        }
        dir = strtok_r(nullptr, ":", &saveptr);
    }
}

static void collect_command_matches(const char* prefix, char** matches, int* count) {
    collect_builtin_matches(prefix, matches, count);
    collect_path_matches(prefix, matches, count);
}

static void collect_file_matches(const char* prefix, char** matches, int* count) {
    DIR* dp = opendir(".");
    if (dp == nullptr) {
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dp)) != nullptr) {
        if (strncmp(entry->d_name, prefix, strlen(prefix)) == 0) {
            add_match(entry->d_name, matches, count);
        }
    }
    closedir(dp);
}

static int common_prefix_len(char** matches, int count) {
    if (count == 0) {
        return 0;
    }
    int len = strlen(matches[0]);
    for (int i = 1; i < count; i++) {
        int j = 0;
        while (j < len && matches[i][j] != '\0' && matches[0][j] == matches[i][j]) {
            j++;
        }
        len = j;
    }
    return len;
}

static void handle_tab(char* buffer, int* pos, int max_len, bool* tab_list_shown) {
    int word_start = find_word_start(buffer, *pos);
    char prefix[MAX_NAME_LEN];
    int prefix_len = *pos - word_start;
    if (prefix_len >= MAX_NAME_LEN) {
        prefix_len = MAX_NAME_LEN - 1;
    }
    strncpy(prefix, buffer + word_start, prefix_len);
    prefix[prefix_len] = '\0';

    char* matches[MAX_MATCHES];
    int match_count = 0;

    if (is_completing_command(buffer, word_start)) {
        collect_command_matches(prefix, matches, &match_count);
    } else {
        collect_file_matches(prefix, matches, &match_count);
    }

    if (match_count == 0) {
        free_matches(matches, match_count);
        return;
    }

    if (match_count == 1) {
        const char* completion = matches[0];
        int comp_len = strlen(completion);
        for (int i = prefix_len; i < comp_len && *pos < max_len - 1; i++) {
            buffer[*pos] = completion[i];
            (*pos)++;
            std::cout << completion[i];
            std::cout.flush();
        }
        buffer[*pos] = '\0';
        *tab_list_shown = false;
    } else {
        int common_len = common_prefix_len(matches, match_count);
        if (common_len > prefix_len) {
            for (int i = prefix_len; i < common_len && *pos < max_len - 1; i++) {
                buffer[*pos] = matches[0][i];
                (*pos)++;
                std::cout << matches[0][i];
            std::cout.flush();
            }
            buffer[*pos] = '\0';
            *tab_list_shown = false;
        } else if (!*tab_list_shown) {
            std::cout << '\n';
            for (int i = 0; i < match_count; i++) {
                std::cout << matches[i];
                if (i + 1 < match_count) {
                    std::cout << ' ';
                }
            }
            std::cout << '\n';
            std::cout.flush();
            display_prompt();
            std::cout << buffer;
            std::cout.flush();
            *tab_list_shown = true;
        }
    }

    free_matches(matches, match_count);
}

static void redraw_line(const char* buffer) {
    write_str("\r");
    display_prompt();
    std::cout << buffer << "\033[K";
    std::cout.flush();
}

static void set_line_from_history(char* buffer, int* pos, int max_len, int history_index) {
    const char* entry = get_history_entry(history_index);
    strncpy(buffer, entry, max_len - 1);
    buffer[max_len - 1] = '\0';
    *pos = strlen(buffer);
    redraw_line(buffer);
}

static void handle_history_up(char* buffer, int* pos, int max_len, int* history_browse) {
    int count = get_history_count();
    if (count == 0) {
        return;
    }

    if (*history_browse == -1) {
        // Start from the most recent command (pairs with DOWN clearing to empty).
        *history_browse = count - 1;
    } else if (*history_browse > 0) {
        (*history_browse)--;
    }

    set_line_from_history(buffer, pos, max_len, *history_browse);
}

static void handle_history_down(char* buffer, int* pos, int max_len, int* history_browse) {
    if (*history_browse == -1) {
        return;
    }

    int count = get_history_count();
    if (*history_browse < count - 1) {
        (*history_browse)++;
        set_line_from_history(buffer, pos, max_len, *history_browse);
    }
}

static void handle_escape_sequence(char* buffer, int* pos, int max_len, int* history_browse,
                                   bool* tab_list_shown) {
    char seq_buf[16];
    int seq_len = 0;

    if (read(STDIN_FILENO, seq_buf, 1) != 1) {
        return;
    }
    seq_len = 1;

    // Collect any immediately available follow-up bytes (handles \e[A, \e[1;3A, \eOA).
    while (seq_len < static_cast<int>(sizeof(seq_buf) - 1)) {
        fd_set fds;
        struct timeval tv = {0, 50000};
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0) {
            break;
        }
        if (read(STDIN_FILENO, seq_buf + seq_len, 1) != 1) {
            break;
        }
        seq_len++;
        char last = seq_buf[seq_len - 1];
        if (last == 'A' || last == 'B' || last == 'C' || last == 'D') {
            break;
        }
    }

    char arrow = 0;
    if (seq_buf[0] == 'O' && seq_len >= 2) {
        arrow = seq_buf[1];
    } else if (seq_buf[0] == '[') {
        for (int i = 1; i < seq_len; i++) {
            if (seq_buf[i] == 'A' || seq_buf[i] == 'B' || seq_buf[i] == 'C' || seq_buf[i] == 'D') {
                arrow = seq_buf[i];
                break;
            }
        }
    }

    if (arrow == 'A') {
        handle_history_up(buffer, pos, max_len, history_browse);
        *tab_list_shown = false;
    } else if (arrow == 'B') {
        handle_history_down(buffer, pos, max_len, history_browse);
        *tab_list_shown = false;
    }
}

bool read_line_with_autocomplete(char* buffer, int max_len) {
    if (max_len <= 0) {
        return false;
    }

    if (!isatty(STDIN_FILENO)) {
        if (std::cin.getline(buffer, max_len)) {
            return true;
        }
        return false;
    }

    enable_raw_mode();

    int pos = 0;
    buffer[0] = '\0';
    bool tab_list_shown = false;
    int history_browse = -1;

    while (true) {
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n <= 0) {
            restore_terminal();
            return false;
        }

        if (c == '\n' || c == '\r') {
            buffer[pos] = '\0';
            write_str("\n");
            restore_terminal();
            return true;
        }

        if (c == 4) {  // Ctrl-D
            if (pos == 0) {
                restore_terminal();
                return false;
            }
            continue;
        }

        if (c == 27) {  // Escape sequence (arrow keys)
            handle_escape_sequence(buffer, &pos, max_len, &history_browse, &tab_list_shown);
            continue;
        }

        if (c == '\t') {
            handle_tab(buffer, &pos, max_len, &tab_list_shown);
            history_browse = -1;
            continue;
        }

        if (c == 127 || c == 8) {  // Backspace
            if (pos > 0) {
                pos--;
                buffer[pos] = '\0';
                write_str("\b \b");
            }
            tab_list_shown = false;
            history_browse = -1;
            continue;
        }

        if (c >= 32 && pos < max_len - 1) {
            buffer[pos] = c;
            pos++;
            buffer[pos] = '\0';
            write_char(c);
            tab_list_shown = false;
            history_browse = -1;
        }
    }
}
