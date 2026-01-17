#ifndef LOG_H
#define LOG_H

void init_log();

void add_to_log(const char *command);

void handle_log_command(const char *args);

int contains_log_command(const char *command);

#endif
