#include "hop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>

static int resolve_single_path(const char *arg, char *cwd, char *home_dir, char *previous_dir)
{
    char resolved_path[4096];
    char temp_cwd[4096];

    strcpy(temp_cwd, cwd);

    if (strcmp(arg, "~") == 0)
    {

        strcpy(resolved_path, home_dir);
    }
    else if (strcmp(arg, ".") == 0)
    {

        return 0;
    }
    else if (strcmp(arg, "..") == 0)
    {

        char temp[4096];
        strncpy(temp, cwd, sizeof(temp));
        temp[sizeof(temp) - 1] = '\0';
        strcpy(resolved_path, dirname(temp));
    }
    else if (strcmp(arg, "-") == 0)
    {

        if (previous_dir[0] == '\0')
        {

            return 0;
        }
        strcpy(resolved_path, previous_dir);
    }
    else
    {

        if (arg[0] == '/')
        {

            strcpy(resolved_path, arg);
        }
        else
        {
            snprintf(resolved_path, sizeof(resolved_path), "%s/%s", cwd, arg);
        }
    }

    if (chdir(resolved_path) != 0)
    {
        printf("No such directory!\n");
        return -1;
    }

    if (strcmp(arg, "-") == 0)
    {

        char swap[4096];
        strcpy(swap, cwd);
        strcpy(cwd, previous_dir);
        strcpy(previous_dir, swap);
    }
    else
    {
        strcpy(previous_dir, temp_cwd);

        if (getcwd(resolved_path, sizeof(resolved_path)) != NULL)
        {
            strcpy(cwd, resolved_path);
        }
        else
        {
            strcpy(cwd, resolved_path);
        }
    }

    return 0;
}

int check_first_word(const char *input, char *cwd, char *home_dir, char *previous_dir)
{
    char buf[1024];
    strncpy(buf, input, sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';

    char *first = strtok(buf, " \t\r\n");
    if (first && strcmp(first, "hop") == 0)
    {
        char *arg = strtok(NULL, " \t\r\n");

        if (arg == NULL)
        {
            return resolve_single_path("~", cwd, home_dir, previous_dir);
        }

        while (arg != NULL)
        {
            if (resolve_single_path(arg, cwd, home_dir, previous_dir) != 0)
            {

                return -1;
            }
            arg = strtok(NULL, " \t\r\n");
        }
    }

    return 0;
}
