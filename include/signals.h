#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>

extern volatile pid_t fg_pid;

struct BackgroundJob {
    int job_id;
    pid_t pid;
    char command[256];
    bool is_running;
};

void init_signal_handlers();

// Background job tracking & process management
void add_job(pid_t pid, const char* cmd);
void remove_job(pid_t pid);
void update_job_status(pid_t pid, bool is_running);
void execute_activities();
void execute_ping(char** args, int arg_count);
void execute_fg(char** args, int arg_count);
void execute_bg(char** args, int arg_count);

#endif