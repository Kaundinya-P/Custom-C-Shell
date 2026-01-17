#define _POSIX_C_SOURCE 200809L
#include "sequential_execution.h"
#include "command_execution.h"
#include "background_execution.h"
#include "hop.h"
#include "reveal.h"
#include "log.h"
#include "activities.h"
#include "ping.h"
#include "job_control.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

static void trim_inplace(char *s)
{
    if (!s)
        return;

    char *p = s;
    while (*p && isspace((unsigned char)*p))
        p++;
    if (p != s)
        memmove(s, p, strlen(p) + 1);

    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1]))
        s[--n] = '\0';
}

static void process_single_command(char *cmd)
{
    if (!cmd || cmd[0] == '\0')
        return;

    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        strcpy(cwd, ".");
    }

    char *home_dir = getenv("HOME");
    if (!home_dir)
    {
        home_dir = cwd;
    }

    static char previous_dir[4096] = "";

    char *cmd_copy = strdup(cmd);
    if (!cmd_copy)
        return;
    char *first = strtok(cmd_copy, " \t\r\n");
    if (!first)
    {
        free(cmd_copy);
        return;
    }
    // to skip built-in commands
    if (strcmp(first, "hop") == 0)
    {
        check_first_word(cmd, cwd, home_dir, previous_dir);
    }
    else if (strcmp(first, "reveal") == 0)
    {
        print_the_files(cmd, cwd, home_dir, previous_dir);
    }
    else if (strcmp(first, "log") == 0)
    {
        char *args = cmd + 3;
        while (*args == ' ' || *args == '\t')
            args++;
        handle_log_command(*args ? args : NULL);
    }
    else if (strcmp(first, "activities") == 0)
    {
        handle_activities_command();
    }
    else if (strcmp(first, "ping") == 0)
    {
        char *args = cmd + 4;
        while (*args == ' ' || *args == '\t')
            args++;
        handle_ping_command(*args ? args : NULL);
    }
    else if (strcmp(first, "fg") == 0)
    {
        char *args = cmd + 2;
        while (*args == ' ' || *args == '\t')
            args++;
        handle_fg_command(*args ? args : NULL);
    }
    else if (strcmp(first, "bg") == 0)
    {
        char *args = cmd + 2;
        while (*args == ' ' || *args == '\t')
            args++;
        handle_bg_command(*args ? args : NULL);
    }
    else
    {

        execute_command(cmd);
    }

    free(cmd_copy);
}

static void execute_segment(char *segment)
{
    if (!segment)
        return;

    char *copy = strdup(segment);
    if (!copy)
        return;

    char *saveptr = NULL;
    char *tok = strtok_r(copy, "&", &saveptr);
    while (tok != NULL)
    {
        char *cmd = strdup(tok);
        if (cmd)
        {
            trim_inplace(cmd);
            if (cmd[0] != '\0')
            {

                char *next_tok = strtok_r(NULL, "&", &saveptr);
                if (next_tok != NULL)
                {

                    execute_background(cmd);

                    tok = next_tok;
                    free(cmd);
                    continue;
                }
                else
                {
                    process_single_command(cmd);
                }
            }
            free(cmd);
        }
        tok = strtok_r(NULL, "&", &saveptr);
    }

    free(copy);
}

void execute_sequential(char *input)
{
    if (!input)
        return;
    char *copy = strdup(input);
    if (!copy)
        return;

    char *saveptr = NULL;
    char *tok = strtok_r(copy, ";", &saveptr);
    while (tok != NULL)
    {
        char *segment = strdup(tok);
        if (segment)
        {
            trim_inplace(segment);
            if (segment[0] != '\0')
            {
                execute_segment(segment);
            }
            free(segment);
        }
        tok = strtok_r(NULL, ";", &saveptr);
    }

    free(copy);
}
