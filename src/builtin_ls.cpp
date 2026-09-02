#include "builtin_ls.h"
#include "prompt.h"
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <cstring>
#include <unistd.h>
#include <climits>

static void print_permissions(mode_t mode) {
    std::cout << (S_ISDIR(mode) ? 'd' : S_ISLNK(mode) ? 'l' : '-');
    std::cout << ((mode & S_IRUSR) ? 'r' : '-');
    std::cout << ((mode & S_IWUSR) ? 'w' : '-');
    std::cout << ((mode & S_IXUSR) ? 'x' : '-');
    std::cout << ((mode & S_IRGRP) ? 'r' : '-');
    std::cout << ((mode & S_IWGRP) ? 'w' : '-');
    std::cout << ((mode & S_IXGRP) ? 'x' : '-');
    std::cout << ((mode & S_IROTH) ? 'r' : '-');
    std::cout << ((mode & S_IWOTH) ? 'w' : '-');
    std::cout << ((mode & S_IXOTH) ? 'x' : '-');
}

static void print_file_info(const char* full_path, const char* name) {
    struct stat file_stat;
    if (lstat(full_path, &file_stat) < 0) {
        perror("ls lstat error");
        return;
    }

    print_permissions(file_stat.st_mode);

    struct passwd* pw = getpwuid(file_stat.st_uid);
    struct group* gr = getgrgid(file_stat.st_gid);

    std::cout << " " << file_stat.st_nlink << " "
              << (pw ? pw->pw_name : "unknown") << " "
              << (gr ? gr->gr_name : "unknown") << " "
              << file_stat.st_size << " ";

    char time_buf[64];
    struct tm* time_info = localtime(&file_stat.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", time_info);
    std::cout << time_buf << " " << name << "\n";
}

static void list_directory(const char* path, bool flag_a, bool flag_l, bool multiple_targets) {
    DIR* dir = opendir(path);
    if (!dir) {
        // Check if target is a file instead of a directory
        struct stat st;
        if (lstat(path, &st) == 0) {
            if (flag_l) {
                print_file_info(path, path);
            } else {
                std::cout << path << "\n";
            }
            return;
        }
        perror("ls error");
        return;
    }

    if (multiple_targets) {
        std::cout << path << ":\n";
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (!flag_a && entry->d_name[0] == '.') {
            continue;
        }

        if (flag_l) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
            print_file_info(full_path, entry->d_name);
        } else {
            std::cout << entry->d_name << "\n";
        }
    }

    closedir(dir);
}

static const char* resolve_path(const char* path, char* resolved, size_t resolved_size) {
    if (strcmp(path, "~") == 0) {
        return SHELL_HOME;
    }
    if (path[0] == '~' && path[1] == '/') {
        snprintf(resolved, resolved_size, "%s%s", SHELL_HOME, path + 1);
        return resolved;
    }
    return path;
}

void execute_ls(char** args, int arg_count) {
    bool flag_a = false;
    bool flag_l = false;

    char* targets[128];
    int target_count = 0;

    for (int i = 1; i < arg_count; ++i) {
        if (args[i][0] == '-' && strlen(args[i]) > 1) {
            for (size_t j = 1; j < strlen(args[i]); ++j) {
                if (args[i][j] == 'a') flag_a = true;
                else if (args[i][j] == 'l') flag_l = true;
            }
        } else {
            targets[target_count++] = args[i];
        }
    }

    if (target_count == 0) {
        list_directory(".", flag_a, flag_l, false);
    } else {
        char resolved[PATH_MAX];
        for (int i = 0; i < target_count; ++i) {
            const char* path = resolve_path(targets[i], resolved, sizeof(resolved));
            list_directory(path, flag_a, flag_l, target_count > 1);
            if (i < target_count - 1) std::cout << "\n";
        }
    }
}