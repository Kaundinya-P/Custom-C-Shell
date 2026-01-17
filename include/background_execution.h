#ifndef BACKGROUND_EXECUTION_H
#define BACKGROUND_EXECUTION_H

#include <sys/types.h>

void execute_background(char *cmd);

int check_background_jobs(void);
int strip_background(char *input);

#endif 
