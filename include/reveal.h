#ifndef REVEAL_H
#define REVEAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>

int print_the_files( const char* input,char* dir_name, char* home_dir, char* previous_dir);
int cmpstring(const void *a, const void *b);

#endif