// search builtin - find file/folder recursively

#include "builtin_search.h"
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <climits>

// recursively check all files and subdirs
static bool searchdir(const char* dirpath, const char* name) {
    DIR* dir = opendir(dirpath);
    if (!dir) {
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // found it
        if (strcmp(entry->d_name, name) == 0) {
            closedir(dir);
            return true;
        }

        char fullpath[PATH_MAX];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);

        // go into subdirectories
        struct stat st;
        if (lstat(fullpath, &st) == 0 && S_ISDIR(st.st_mode)) {
            if (searchdir(fullpath, name)) {
                closedir(dir);
                return true;
            }
        }
    }

    closedir(dir);
    return false;
}

void executesearch(char** args, int argc) {
    if (argc != 2) {
        std::cout << "Usage: search <file_or_folder_name>\n";
        return;
    }

    if (searchdir(".", args[1])) {
        std::cout << "True\n";
    } else {
        std::cout << "False\n";
    }
}
