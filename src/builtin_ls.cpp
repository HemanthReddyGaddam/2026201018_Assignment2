// ls builtin - list files with -a and -l support

#include "builtin_ls.h"
#include "prompt.h"
#include<iostream>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<pwd.h>
#include<grp.h>
#include<time.h>
#include<cstring>
#include<unistd.h>
#include<climits>

// print rwxrwxrwx style permissions
static void printperms(mode_t mode) {
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

// print one line for ls -l (perms, owner, size, date, name)
static void printfileinfo(const char* fullpath, const char* name) {
    struct stat st;
    if (lstat(fullpath, &st) < 0) {
        perror("ls lstat error");
        return;
    }

    printperms(st.st_mode);

    struct passwd* pw = getpwuid(st.st_uid);
    struct group* gr = getgrgid(st.st_gid);

    std::cout << " " << st.st_nlink << " "
              << (pw ? pw->pw_name : "unknown") << " "
              << (gr ? gr->gr_name : "unknown") << " "
              << st.st_size<< " ";

    char timebuf[64];
    struct tm* t = localtime(&st.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", t);
    std::cout << timebuf << " " << name<< "\n";
}

// list contents of one directory
static void listdir(const char* path, bool showall, bool longfmt, bool many) {
    DIR* dir = opendir(path);
    if (!dir) {
        // maybe it's a file, not a folder
        struct stat st;
        if (lstat(path, &st) == 0) {
            if (longfmt) {
                printfileinfo(path, path);
            } else {
                std::cout << path << "\n";
            }
            return;
        }
        perror("ls error");
        return;
    }

    // print dir name when listing multiple targets
    if (many) {
        std::cout << path << ":\n";
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // skip hidden files unless -a
        if (!showall && entry->d_name[0] == '.') {
            continue;
        }

        if (longfmt) {
            char fullpath[PATH_MAX];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
            printfileinfo(fullpath, entry->d_name);
        } else {
            std::cout << entry->d_name<< "\n";
        }
    }

    closedir(dir);
}

// handle ~ in paths like ~/test
static const char* resolvepath(const char* path, char* buf, int buflen) {
    if (strcmp(path, "~") == 0) {
        return shell_home;
    }
    if (path[0] == '~' && path[1] == '/') {
        snprintf(buf, buflen, "%s%s", shell_home, path + 1);
        return buf;
    }
    return path;
}

void executels(char** args, int argc) {
    bool showall = false;
    bool longfmt = false;

    char* targets[128];
    int targetcount = 0;

    // parse flags and directory names from args
    for (int i = 1; i < argc; i++) {
        if (args[i][0] == '-' && strlen(args[i]) > 1) {
            // handle -a, -l, -al, -la etc
            for (size_t j = 1; j < strlen(args[i]); j++) {
                if (args[i][j] == 'a') {
                    showall = true;
                } else if (args[i][j] == 'l') {
                    longfmt = true;
                }
            }
        } else {
            targets[targetcount++] = args[i];
        }
    }

    if (targetcount == 0) {
        listdir(".", showall, longfmt, false);
    } else {
        char resolved[PATH_MAX];
        for (int i = 0; i < targetcount; i++) {
            const char* path = resolvepath(targets[i], resolved, sizeof(resolved));
            listdir(path, showall, longfmt, targetcount > 1);
            if (i < targetcount - 1) {
                std::cout << "\n";
            }
        }
    }
}
