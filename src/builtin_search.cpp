#include "builtin_search.h"
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <climits>

static bool recursive_search(const char* current_dir, const char* target_name) {
    DIR* dir = opendir(current_dir);
    if (!dir) return false;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (strcmp(entry->d_name, target_name) == 0) {
            closedir(dir);
            return true;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, entry->d_name);

        struct stat st;
        if (lstat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            if (recursive_search(full_path, target_name)) {
                closedir(dir);
                return true;
            }
        }
    }

    closedir(dir);
    return false;
}

void execute_search(char** args, int arg_count) {
    if (arg_count != 2) {
        std::cout << "Usage: search <file_or_folder_name>\n";
        return;
    }

    bool found = recursive_search(".", args[1]);
    if (found) {
        std::cout << "True\n";
    } else {
        std::cout << "False\n";
    }
}