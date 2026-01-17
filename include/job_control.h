#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <sys/types.h>

void handle_fg_command(const char *args);

void handle_bg_command(const char *args);

void add_job(pid_t pid, const char *command);
void remove_job(pid_t pid);
void update_job_state(pid_t pid, const char *state);
pid_t get_most_recent_job();

#endif
