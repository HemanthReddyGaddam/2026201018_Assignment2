#include "builtin_pinfo.h"
#include "prompt.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <climits>
#include <sys/types.h>

#ifdef __APPLE__
#include <libproc.h>
#include <sys/proc_info.h>
#endif

static void print_executable_path(const char* exe_path) {
    size_t home_len = strlen(SHELL_HOME);
    if (home_len > 0 && strncmp(exe_path, SHELL_HOME, home_len) == 0 &&
        (exe_path[home_len] == '\0' || exe_path[home_len] == '/')) {
        std::cout << "Executable Path -- ~" << (exe_path + home_len) << "\n";
    } else {
        std::cout << "Executable Path -- " << exe_path << "\n";
    }
}

void execute_pinfo(char** args, int arg_count) {
    pid_t pid = getpid();
    if (arg_count > 1) {
        pid = atoi(args[1]);
    }

#ifdef __linux__
    char stat_path[64];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);

    FILE* stat_file = fopen(stat_path, "r");
    if (stat_file == nullptr) {
        perror("pinfo error");
        return;
    }

    char line[4096];
    if (fgets(line, sizeof(line), stat_file) == nullptr) {
        perror("pinfo error");
        fclose(stat_file);
        return;
    }
    fclose(stat_file);

    char* rparen = strrchr(line, ')');
    if (rparen == nullptr) {
        std::cout << "pinfo error: could not parse process state\n";
        return;
    }

    char* fields_start = rparen + 2;
    while (*fields_start == ' ') {
        fields_start++;
    }

    char state = fields_start[0];
    pid_t pgrp = 0;
    pid_t tpgid = 0;
    unsigned long vsize = 0;

    char* field = fields_start;
    int field_num = 3;
    while (*field != '\0' && field_num <= 23) {
        if (field_num == 5) {
            pgrp = atoi(field);
        } else if (field_num == 8) {
            tpgid = atoi(field);
        } else if (field_num == 23) {
            vsize = strtoul(field, nullptr, 10);
        }

        while (*field != '\0' && *field != ' ') {
            field++;
        }
        while (*field == ' ') {
            field++;
        }
        field_num++;
    }

    char status[8];
    status[0] = state;
    status[1] = '\0';
    if (pgrp == tpgid && tpgid != -1) {
        status[1] = '+';
        status[2] = '\0';
    }

    std::cout << "Process Status -- {" << status << "}\n";
    std::cout << "memory -- " << vsize << " {Virtual Memory}\n";

    char exe_link[64];
    snprintf(exe_link, sizeof(exe_link), "/proc/%d/exe", pid);
    char exe_path[PATH_MAX];
    ssize_t len = readlink(exe_link, exe_path, sizeof(exe_path) - 1);

    if (len != -1) {
        exe_path[len] = '\0';
        print_executable_path(exe_path);
    } else {
        std::cout << "Executable Path -- Permission Denied / Not Found\n";
    }

#elif defined(__APPLE__)
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
    std::cout << "memory -- " << vsize << " {Virtual Memory}\n";

    char exe_path[PATH_MAX];
    int ret = proc_pidpath(pid, exe_path, sizeof(exe_path));
    if (ret > 0) {
        print_executable_path(exe_path);
    } else {
        std::cout << "Executable Path -- Permission Denied / Not Found\n";
    }
#endif
}
