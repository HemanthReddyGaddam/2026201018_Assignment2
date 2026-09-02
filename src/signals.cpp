#include "signals.h"
#include "prompt.h"
#include <iostream>
#include <cstring>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>

volatile pid_t fg_pid = 0;

#define MAX_JOBS 256

static BackgroundJob jobs[MAX_JOBS];
static int job_count = 0;
static int next_job_id = 1;

void add_job(pid_t pid, const char* cmd) {
    if (job_count >= MAX_JOBS) {
        return;
    }

    BackgroundJob& job = jobs[job_count++];
    job.job_id = next_job_id++;
    job.pid = pid;
    strncpy(job.command, cmd, sizeof(job.command) - 1);
    job.command[sizeof(job.command) - 1] = '\0';
    job.is_running = true;
}

void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i].pid == pid) {
            for (int j = i; j < job_count - 1; ++j) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            return;
        }
    }
}

void update_job_status(pid_t pid, bool is_running) {
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i].pid == pid) {
            jobs[i].is_running = is_running;
            return;
        }
    }
}

static int compare_jobs(const void* a, const void* b) {
    const BackgroundJob* ja = static_cast<const BackgroundJob*>(a);
    const BackgroundJob* jb = static_cast<const BackgroundJob*>(b);
    return strcmp(ja->command, jb->command);
}

void execute_activities() {
    qsort(jobs, job_count, sizeof(BackgroundJob), compare_jobs);

    for (int i = 0; i < job_count; ++i) {
        std::cout << jobs[i].pid << " : " << jobs[i].command << " - "
                  << (jobs[i].is_running ? "Running" : "Stopped") << "\n";
    }
}

void execute_ping(char** args, int arg_count) {
    if (arg_count < 3) {
        std::cout << "Usage: ping <pid> <signal_number>\n";
        return;
    }

    pid_t pid = atoi(args[1]);
    int signal_num = atoi(args[2]);

    int sig = signal_num % 32;
    if (kill(pid, sig) < 0) {
        perror("Ping failed");
    } else {
        std::cout << "Sent signal " << sig << " to process with pid " << pid << "\n";
    }
}

static BackgroundJob* find_job(pid_t pid) {
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i].pid == pid) {
            return &jobs[i];
        }
    }
    return nullptr;
}

void execute_fg(char** args, int arg_count) {
    if (arg_count < 2) {
        std::cout << "Usage: fg <pid>\n";
        return;
    }

    pid_t pid = atoi(args[1]);
    if (find_job(pid) == nullptr) {
        std::cout << "No process with PID " << pid << " found in background\n";
        return;
    }

    kill(pid, SIGCONT);

    int status;
    fg_pid = pid;
    waitpid(pid, &status, WUNTRACED);
    fg_pid = 0;
    remove_job(pid);
}

void execute_bg(char** args, int arg_count) {
    if (arg_count < 2) {
        std::cout << "Usage: bg <pid>\n";
        return;
    }

    pid_t pid = atoi(args[1]);
    if (find_job(pid) == nullptr) {
        std::cout << "No process with PID " << pid << " found in background\n";
        return;
    }

    if (kill(pid, SIGCONT) < 0) {
        perror("bg failed");
    } else {
        update_job_status(pid, true);
    }
}

static void sigint_handler(int sig) {
    (void)sig;
    if (fg_pid > 0) {
        kill(-fg_pid, SIGINT);
    } else {
        std::cout << "\n";
    }
}

static void sigtstp_handler(int sig) {
    (void)sig;
    if (fg_pid > 0) {
        kill(-fg_pid, SIGTSTP);
    } else {
        std::cout << "\n";
        std::cout.flush();
    }
}

static void sigchld_handler(int sig) {
    (void)sig;
    int status;
    pid_t pid;

    for (int i = 0; i < job_count; ) {
        pid = waitpid(jobs[i].pid, &status, WNOHANG);
        if (pid > 0) {
            std::cout << "\nProcess with PID " << pid << " exited cleanly\n";
            for (int j = i; j < job_count - 1; ++j) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            displayprompt();
            std::cout.flush();
        } else {
            ++i;
        }
    }
}

void init_signal_handlers() {
    struct sigaction sa_int{}, sa_tstp{}, sa_chld{};

    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, nullptr);

    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, nullptr);

    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, nullptr);
}
