#include "executor.h"
#include "signals.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstring>

void execute_system_command(char** args, int arg_count, bool is_background) {
    (void)arg_count;

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
        setpgid(0, 0);
        if (execvp(args[0], args) < 0) {
            std::cout << "ERROR: '" << args[0] << "' is not a valid command\n";
            _exit(EXIT_FAILURE);
        }
    } else {
        setpgid(pid, pid);
        if (is_background) {
            std::cout << pid << "\n";
            add_job(pid, args[0]);
        } else {
            fg_pid = pid;
            int status;
            waitpid(pid, &status, WUNTRACED);
            if (WIFSTOPPED(status)) {
                add_job(pid, args[0]);
                update_job_status(pid, false);
                std::cout << "\n[" << pid << "] Stopped\n";
            }
            fg_pid = 0;
        }
    }
}