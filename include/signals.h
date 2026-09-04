#ifndef SIGNALS_H
#define SIGNALS_H

#include<sys/types.h>

// PID of whatever process is currently eating up the foreground
extern volatile pid_t fg_pid;

// Simple tracker for tasks running in the background
struct bgjob {
    int jobid;
    pid_t pid;
    char command[256];
    bool running;
};

// Setup signal traps, manage jobs, and handle process control commands
void initsignalhandlers();
void addjob(pid_t pid, const char* cmd);
void removejob(pid_t pid);
void updatejobstatus(pid_t pid, bool running);
void executeactivities();
void executeping(char** args, int argc);
void executefg(char** args, int argc);
void executebg(char** args, int argc);

#endif