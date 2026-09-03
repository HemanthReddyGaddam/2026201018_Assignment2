// signal handling - ctrl-c, ctrl-z, background jobs

#include "signals.h"
#include "prompt.h"
#include <iostream>
#include <cstring>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>

// pid of whatever is running in foreground right now
volatile pid_t fg_pid = 0;

#define max_jobs 256

static bgjob jobs[max_jobs];
static int jobcount = 0;
static int nextjobid = 1;

void addjob(pid_t pid, const char* cmd) {
    if (jobcount >= max_jobs) {
        return;
    }

    jobs[jobcount].jobid = nextjobid++;
    jobs[jobcount].pid = pid;
    strncpy(jobs[jobcount].command, cmd, sizeof(jobs[jobcount].command) - 1);
    jobs[jobcount].command[sizeof(jobs[jobcount].command) - 1] = '\0';
    jobs[jobcount].running = true;
    jobcount++;
}

void removejob(pid_t pid) {
    for (int i = 0; i < jobcount; i++) {
        if (jobs[i].pid == pid) {
            // shift remaining jobs down
            for (int j = i; j < jobcount - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            jobcount--;
            return;
        }
    }
}

void updatejobstatus(pid_t pid, bool running) {
    for (int i = 0; i < jobcount; i++) {
        if (jobs[i].pid == pid) {
            jobs[i].running = running;
            return;
        }
    }
}

static int comparejobs(const void* a, const void* b) {
    const bgjob* ja = (const bgjob*)a;
    const bgjob* jb = (const bgjob*)b;
    return strcmp(ja->command, jb->command);
}

// list all background/stopped jobs
void executeactivities() {
    qsort(jobs, jobcount, sizeof(bgjob), comparejobs);
    for (int i = 0; i < jobcount; i++) {
        std::cout << jobs[i].pid << " : " << jobs[i].command << " - "
                  << (jobs[i].running ? "Running" : "Stopped") << "\n";
    }
}

// send a signal to a process
void executeping(char** args, int argc) {
    if (argc < 3) {
        std::cout << "Usage: ping <pid> <signal_number>\n";
        return;
    }

    pid_t pid = atoi(args[1]);
    int signum = atoi(args[2]);
    int sig = signum % 32;

    if (kill(pid, sig) < 0) {
        perror("Ping failed");
    } else {
        std::cout << "Sent signal " << sig << " to process with pid " << pid << "\n";
    }
}

static bgjob* findjob(pid_t pid) {
    for (int i = 0; i < jobcount; i++) {
        if (jobs[i].pid == pid) {
            return &jobs[i];
        }
    }
    return nullptr;
}

// bring a background job to foreground
void executefg(char** args, int argc) {
    if (argc < 2) {
        std::cout << "Usage: fg <pid>\n";
        return;
    }

    pid_t pid = atoi(args[1]);
    if (findjob(pid) == nullptr) {
        std::cout << "No process with PID " << pid << " found in background\n";
        return;
    }

    kill(pid, SIGCONT);
    fg_pid = pid;
    int status;
    waitpid(pid, &status, WUNTRACED);
    fg_pid = 0;
    removejob(pid);
}

// resume a stopped background job
void executebg(char** args, int argc) {
    if (argc < 2) {
        std::cout << "Usage: bg <pid>\n";
        return;
    }

    pid_t pid = atoi(args[1]);
    if (findjob(pid) == nullptr) {
        std::cout << "No process with PID " << pid << " found in background\n";
        return;
    }

    if (kill(pid, SIGCONT) < 0) {
        perror("bg failed");
    } else {
        updatejobstatus(pid, true);
    }
}

// ctrl-c handler
static void siginthandler(int sig) {
    (void)sig;
    if (fg_pid > 0) {
        kill(-fg_pid, SIGINT);
    } else {
        std::cout << "\n";
    }
}

// ctrl-z handler
static void sigtstphandler(int sig) {
    (void)sig;
    if (fg_pid > 0) {
        kill(-fg_pid, SIGTSTP);
    } else {
        std::cout << "\n";
        std::cout.flush();
    }
}

// called when a background child exits
static void sigchldhandler(int sig) {
    (void)sig;
    int status;
    pid_t pid;

    for (int i = 0; i < jobcount; ) {
        pid = waitpid(jobs[i].pid, &status, WNOHANG);
        if (pid > 0) {
            std::cout << "\nProcess with PID " << pid << " exited cleanly\n";
            for (int j = i; j < jobcount - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            jobcount--;
            displayprompt();
            std::cout.flush();
        } else {
            i++;
        }
    }
}

void initsignalhandlers() {
    struct sigaction sint{}, stp{}, chld{};

    sint.sa_handler = siginthandler;
    sigemptyset(&sint.sa_mask);
    sint.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sint, nullptr);

    stp.sa_handler = sigtstphandler;
    sigemptyset(&stp.sa_mask);
    stp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &stp, nullptr);

    chld.sa_handler = sigchldhandler;
    sigemptyset(&chld.sa_mask);
    chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &chld, nullptr);
}
