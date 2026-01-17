#define _POSIX_C_SOURCE 200809L
#include "log.h"
#include "command_execution.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_LOG_SIZE 15
#define LOG_FILE ".shell_log"

static char *log_entries[MAX_LOG_SIZE];
static int log_count = 0;
static int log_start = 0;
static char *home_dir = NULL;

static const char *get_home_dir()
{
    if (!home_dir)
    {
        home_dir = getenv("HOME");
        if (!home_dir)
        {
            home_dir = ".";
        }
    }
    return home_dir;
}

static void get_log_path(char *path, size_t size)
{
    snprintf(path, size, "%s/%s", get_home_dir(), LOG_FILE);
}

static void load_log()
{
    char log_path[4096];
    get_log_path(log_path, sizeof(log_path));

    FILE *file = fopen(log_path, "r");
    if (!file)
        return;

    char line[1024];
    while (fgets(line, sizeof(line), file) && log_count < MAX_LOG_SIZE)
    {

        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0)
        {
            log_entries[log_count] = strdup(line);
            log_count++;
        }
    }
    fclose(file);
}

static void save_log()
{
    char log_path[4096];
    get_log_path(log_path, sizeof(log_path));

    FILE *file = fopen(log_path, "w");
    if (!file)
        return;

    for (int i = 0; i < log_count; i++)
    {
        int idx = (log_start + i) % MAX_LOG_SIZE;
        if (log_entries[idx])
        {
            fprintf(file, "%s\n", log_entries[idx]);
        }
    }
    fclose(file);
}

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

void init_log()
{
    for (int i = 0; i < MAX_LOG_SIZE; i++)
    {
        if (log_entries[i])
        {
            free(log_entries[i]);
            log_entries[i] = NULL;
        }
    }
    log_count = 0;
    log_start = 0;

    load_log();
}

void add_to_log(const char *command)
{
    if (!command || strlen(command) == 0)
        return;

    if (contains_log_command(command))
        return;

    char *trimmed = strdup(command);
    trimmed = trim_whitespace(trimmed);

    if (log_count > 0)
    {
        int last_idx = (log_start + log_count - 1) % MAX_LOG_SIZE;
        if (log_entries[last_idx] && strcmp(log_entries[last_idx], trimmed) == 0)
        {
            free(trimmed);
            return;
        }
    }

    if (log_count < MAX_LOG_SIZE)
    {
        log_entries[log_count] = trimmed;
        log_count++;
    }
    else
    {
        free(log_entries[log_start]);
        log_entries[log_start] = trimmed;
        log_start = (log_start + 1) % MAX_LOG_SIZE;
    }

    save_log();
}
int contains_log_command(const char *command)
{
    if (!command)
        return 0;
    char *copy = strdup(command);
    char *saveptr = NULL;

    char *cmd_group = strtok_r(copy, ";&", &saveptr);
    while (cmd_group)
    {
        char *trimmed = trim_whitespace(cmd_group);

        char *saveptr2 = NULL;
        char *atomic = strtok_r(trimmed, "|", &saveptr2);
        while (atomic)
        {
            atomic = trim_whitespace(atomic);

            char *saveptr3 = NULL;
            char *first_word = strtok_r(atomic, " \t", &saveptr3);
            if (first_word && strcmp(first_word, "log") == 0)
            {
                free(copy);
                return 1;
            }

            atomic = strtok_r(NULL, "|", &saveptr2);
        }

        cmd_group = strtok_r(NULL, ";&", &saveptr);
    }

    free(copy);
    return 0;
}

void handle_log_command(const char *args)
{
    char *args_copy = strdup(args ? args : "");
    char *trimmed_args = trim_whitespace(args_copy);

    if (strlen(trimmed_args) == 0)
    {

        for (int i = 0; i < log_count; i++)
        {
            int idx = (log_start + i) % MAX_LOG_SIZE;
            if (log_entries[idx])
            {
                printf("%s\n", log_entries[idx]);
            }
        }
    }
    else if (strcmp(trimmed_args, "purge") == 0)
    {

        for (int i = 0; i < MAX_LOG_SIZE; i++)
        {
            if (log_entries[i])
            {
                free(log_entries[i]);
                log_entries[i] = NULL;
            }
        }
        log_count = 0;
        log_start = 0;
        save_log();
    }
    else if (strncmp(trimmed_args, "execute ", 8) == 0)
    {

        char *index_str = trimmed_args + 8;
        index_str = trim_whitespace(index_str);

        int index = atoi(index_str);
        if (index < 1 || index > log_count)
        {
            printf("Invalid index\n");
        }
        else
        {
            int actual_idx = (log_start + log_count - index) % MAX_LOG_SIZE;
            if (log_entries[actual_idx])
            {
                char *cmd_to_execute = strdup(log_entries[actual_idx]);
                execute_shell_command(cmd_to_execute);
                free(cmd_to_execute);
            }
        }
    }
    else
    {
        printf("log: Invalid Syntax!\n");
    }

    free(args_copy);
}
