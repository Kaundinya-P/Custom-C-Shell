#ifndef COMMAND_EXECUTION_H
#define COMMAND_EXECUTION_H

void execute_command(char *input);
void execute_shell_command(char *input);
void set_shell_state(char *current_cwd, char *home_directory, char *prev_directory);
void get_shell_state(char *current_cwd, char *home_directory, char *prev_directory);

#endif
