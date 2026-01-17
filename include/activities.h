#ifndef ACTIVITIES_H
#define ACTIVITIES_H

#include <sys/types.h>

void handle_activities_command();

void add_to_activities(pid_t pid, const char *command, int foreground);


void remove_from_activities(pid_t pid);


void update_process_state(pid_t pid, const char *state);

#endif
