// run external commands using fork + execvp

#include "executor.h"
#include "signals.h"
#include<iostream>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<cstring>

void executesystemcmd(char** args, int argc, bool bg) {
    (void)argc;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
        // child process - replace with the actual program
        setpgid(0, 0);
        if (execvp(args[0], args) < 0) {
            std::cout << "ERROR: '" << args[0] << "' is not a valid command\n";
            _exit(EXIT_FAILURE);
        }
    }

    // parent process
    setpgid(pid, pid);
    if (bg) {
        // background - don't wait, just print pid
        std::cout << pid << "\n";
        addjob(pid, args[0]);
    } else {
        // foreground - wait for it to finish
        fg_pid = pid;
        int status;
        waitpid(pid, &status, WUNTRACED);
        if (WIFSTOPPED(status)) {
            // ctrl-z stopped it, move to background
            addjob(pid, args[0]);
            updatejobstatus(pid, false);
            std::cout << "\n[" << pid << "] Stopped\n";
        }
        fg_pid = 0;
    }
}
