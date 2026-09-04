// pinfo builtin - show process info from /proc

#include "builtin_pinfo.h"
#include "prompt.h"
#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<unistd.h>
#include<cstring>
#include<climits>
#include<sys/types.h>

#ifdef __APPLE__
#include<libproc.h>
#include<sys/proc_info.h>
#endif

// print exe path, use ~ if inside shell home
static void printexepath(const char* exepath) {
    size_t homelen = strlen(shell_home);
    if (homelen > 0 && strncmp(exepath, shell_home, homelen) == 0 &&
        (exepath[homelen] == '\0' || exepath[homelen] == '/')) {
        std::cout << "Executable Path -- ~" << (exepath + homelen) << "\n";
    } else {
        std::cout << "Executable Path -- " << exepath << "\n";
    }
}

void executepinfo(char** args, int argc) {
    // default to shell's own pid
    pid_t pid = getpid();
    if (argc > 1) {
        pid = atoi(args[1]);
    }

#ifdef __linux__
    // read /proc/<pid>/stat on linux
    char statpath[64];
    snprintf(statpath, sizeof(statpath), "/proc/%d/stat", pid);

    FILE* fp = fopen(statpath, "r");
    if (fp == nullptr) {
        perror("pinfo error");
        return;
    }

    char line[4096];
    if (fgets(line, sizeof(line), fp) == nullptr) {
        perror("pinfo error");
        fclose(fp);
        return;
    }
    fclose(fp);

    // process name in stat file is inside (parens), so find the closing )
    char* endname = strrchr(line, ')');
    if (endname == nullptr) {
        std::cout << "pinfo error: could not parse process state\n";
        return;
    }

    // fields start after ") "
    char* field = endname + 2;
    while (*field == ' ') {
        field++;
    }

    char state = field[0];
    pid_t pgrp = 0;
    pid_t tpgid = 0;
    unsigned long vsize = 0;

    // walk through stat fields to get what we need
    int fieldnum = 3;
    while (*field != '\0' && fieldnum <= 23) {
        if (fieldnum == 5) {
            pgrp = atoi(field);
        } else if (fieldnum == 8) {
            tpgid = atoi(field);
        } else if (fieldnum == 23) {
            vsize = strtoul(field, nullptr, 10);
        }

        while (*field != '\0' && *field != ' ') {
            field++;
        }
        while (*field == ' ') {
            field++;
        }
        fieldnum++;
    }

    char status[8];
    status[0] = state;
    status[1] = '\0';
    // add + if process is in foreground
    if (pgrp == tpgid && tpgid != -1) {
        status[1] = '+';
        status[2] = '\0';
    }

    std::cout << "Process Status -- {" << status << "}\n";
    std::cout << "memory -- " << vsize<< " {Virtual Memory}\n";

    // get executable path from /proc/<pid>/exe symlink
    char exelink[64];
    snprintf(exelink, sizeof(exelink), "/proc/%d/exe", pid);
    char exepath[PATH_MAX];
    ssize_t len = readlink(exelink, exepath, sizeof(exepath) - 1);

    if (len != -1) {
        exepath[len] = '\0';
        printexepath(exepath);
    } else {
        std::cout << "Executable Path -- Permission Denied / Not Found\n";
    }

#elif defined(__APPLE__)
    // macos uses different api for process info
    struct proc_bsdinfo proc;
    int st = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &proc, sizeof(proc));
    if (st <= 0) {
        std::cout << "pinfo error: process does not exist\n";
        return;
    }

    char status[8] = "S";
    if (proc.pbi_status == 1) {
        status[0] = 'R';
    } else if (proc.pbi_status == 2) {
        status[0] = 'S';
    } else if (proc.pbi_status == 3) {
        status[0] = 'T';
    } else if (proc.pbi_status == 4) {
        status[0] = 'Z';
    }
    status[1] = '\0';

    if (proc.pbi_pgid == static_cast<uint32_t>(tcgetpgrp(STDIN_FILENO))) {
        status[1] = '+';
        status[2] = '\0';
    }

    struct proc_taskinfo task;
    st = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &task, sizeof(task));
    unsigned long vsize = (st > 0) ? task.pti_virtual_size : 0;

    std::cout << "Process Status -- {" << status << "}\n";
    std::cout << "memory -- " << vsize<< " {Virtual Memory}\n";

    char exepath[PATH_MAX];
    int ret = proc_pidpath(pid, exepath, sizeof(exepath));
    if (ret > 0) {
        printexepath(exepath);
    } else {
        std::cout << "Executable Path -- Permission Denied / Not Found\n";
    }
#endif
}
