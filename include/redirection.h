#ifndef REDIRECTION_H
#define REDIRECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
int handle_redirections(const char *input, char *clean_cmd);
#endif