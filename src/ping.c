#define _POSIX_C_SOURCE 200809L
#include "ping.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

static char *trim_whitespace(char *str)
{
    char *end;

    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0)
        return str;

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    end[1] = '\0';
    return str;
}

static int is_valid_number(const char *str)
{
    if (!str || *str == '\0')
        return 0;

    if (*str == '-')
        str++;

    while (*str)
    {
        if (!isdigit(*str))
            return 0;
        str++;
    }
    return 1;
}

void handle_ping_command(const char *args)
{
    if (!args)
    {
        printf("Invalid syntax!\n");
        return;
    }

    char *args_copy = strdup(args);
    char *trimmed_args = trim_whitespace(args_copy);

    char *saveptr = NULL;
    char *pid_str = strtok_r(trimmed_args, " \t", &saveptr);
    char *signal_str = strtok_r(NULL, " \t", &saveptr);
    char *extra = strtok_r(NULL, " \t", &saveptr);

    if (!pid_str || !signal_str || extra)
    {
        printf("Invalid syntax!\n");
        free(args_copy);
        return;
    }

    if (!is_valid_number(pid_str))
    {
        printf("Invalid syntax!\n");
        free(args_copy);
        return;
    }

    if (!is_valid_number(signal_str))
    {
        printf("Invalid syntax!\n");
        free(args_copy);
        return;
    }

    pid_t pid = (pid_t)atoi(pid_str);
    int signal_number = atoi(signal_str);

    int actual_signal = signal_number % 32;

    if (kill(pid, actual_signal) == 0)
    {
        printf("Sent signal %d to process with pid %d\n", signal_number, pid);
    }
    else
    {
        if (errno == ESRCH)
        {
            printf("No such process found\n");
        }
        else
        {
            perror("ping");
        }
    }

    free(args_copy);
}
