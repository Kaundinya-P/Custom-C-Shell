#define _POSIX_C_SOURCE 200809L
#include "job_control.h"
#include "signals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <termios.h>

typedef struct Job
{
    int job_number;
    pid_t pid;
    char *command;
    char *state;
    struct Job *next;
} Job;

static Job *jobs_head = NULL;
static int next_job_number = 1;
static char *trim_whitespace(char *str)
{
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0)
        return str;

    // Trim trailing space
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

    while (*str)
    {
        if (!isdigit(*str))
            return 0;
        str++;
    }
    return 1;
}

static Job *find_job_by_number(int job_number)
{
    Job *cur = jobs_head;
    while (cur)
    {
        if (cur->job_number == job_number)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

static Job *find_job_by_pid(pid_t pid)
{
    Job *cur = jobs_head;
    while (cur)
    {
        if (cur->pid == pid)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

void add_job(pid_t pid, const char *command)
{
    Job *job = malloc(sizeof(Job));
    if (!job)
        return;

    job->job_number = next_job_number++;
    job->pid = pid;
    job->command = strdup(command ? command : "unknown");
    job->state = strdup("Running");
    job->next = jobs_head;
    jobs_head = job;
}

void remove_job(pid_t pid)
{
    Job *prev = NULL;
    Job *cur = jobs_head;

    while (cur)
    {
        if (cur->pid == pid)
        {
            if (prev)
                prev->next = cur->next;
            else
                jobs_head = cur->next;

            free(cur->command);
            free(cur->state);
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

void update_job_state(pid_t pid, const char *state)
{
    Job *job = find_job_by_pid(pid);
    if (job)
    {
        free(job->state);
        job->state = strdup(state);
    }
}

pid_t get_most_recent_job()
{
    if (!jobs_head)
        return -1;

    Job *most_recent = jobs_head;
    Job *cur = jobs_head->next;

    while (cur)
    {
        if (cur->job_number > most_recent->job_number)
        {
            most_recent = cur;
        }
        cur = cur->next;
    }

    return most_recent->pid;
}

void handle_fg_command(const char *args)
{
    Job *job = NULL;

    if (!args || strlen(args) == 0)
    {
        pid_t recent_pid = get_most_recent_job();
        if (recent_pid == -1)
        {
            printf("No such job\n");
            return;
        }
        job = find_job_by_pid(recent_pid);
    }
    else
    {

        char *args_copy = strdup(args);
        char *trimmed = trim_whitespace(args_copy);

        if (!is_valid_number(trimmed))
        {
            printf("No such job\n");
            free(args_copy);
            return;
        }

        int job_number = atoi(trimmed);
        job = find_job_by_number(job_number);
        free(args_copy);

        if (!job)
        {
            printf("No such job\n");
            return;
        }
    }

    printf("%s\n", job->command);
    fflush(stdout);
    //  Give terminal control to the job's process group and set as foreground
    if (isatty(STDIN_FILENO))
    {
        tcsetpgrp(STDIN_FILENO, job->pid);
    }
    set_foreground_process_group(job->pid);

    // If job is stopped, send SIGCONT to resume it (to the whole group)
    if (strcmp(job->state, "Stopped") == 0)
    {
        if (killpg(job->pid, SIGCONT) < 0)
        {
            perror("fg: failed to resume job");
            // Take terminal back on failure
            if (isatty(STDIN_FILENO))
                tcsetpgrp(STDIN_FILENO, getpid());
            clear_foreground_process_group();
            return;
        }
    }

    // Update state to running
    free(job->state);
    job->state = strdup("Running");

    // Wait for the job's process group to complete or stop
    int status;
    for (;;)
    {
        pid_t w = waitpid(-job->pid, &status, WUNTRACED);
        if (w < 0)
        {
            if (errno == EINTR)
                continue;
            // No more children in this group
            break;
        }
        if (WIFSTOPPED(status))
        {
            update_job_state(job->pid, "Stopped");
            printf("[%d] Stopped %s\n", job->job_number, job->command);
            fflush(stdout);
            break;
        }
        if (WIFEXITED(status) || WIFSIGNALED(status))
        {
            // Keep reaping until the group is empty
            continue;
        }
    }

    // Take back terminal control and clear foreground after job state change
    if (isatty(STDIN_FILENO))
    {
        tcsetpgrp(STDIN_FILENO, getpid());
    }
    clear_foreground_process_group();

    // If the group has no children left, remove job
    int tmp;
    if (waitpid(-job->pid, &tmp, WNOHANG) == -1 && errno == ECHILD)
    {
        remove_job(job->pid);
    }
}

void handle_bg_command(const char *args)
{
    Job *job = NULL;

    if (!args || strlen(args) == 0)
    {
        pid_t recent_pid = get_most_recent_job();
        if (recent_pid == -1)
        {
            printf("No such job\n");
            return;
        }
        job = find_job_by_pid(recent_pid);
    }
    else
    {
        char *args_copy = strdup(args);
        char *trimmed = trim_whitespace(args_copy);

        if (!is_valid_number(trimmed))
        {
            printf("No such job\n");
            free(args_copy);
            return;
        }

        int job_number = atoi(trimmed);
        job = find_job_by_number(job_number);
        free(args_copy);

        if (!job)
        {
            printf("No such job\n");
            return;
        }
    }

    if (strcmp(job->state, "Running") == 0)
    {
        printf("Job already running\n");
        return;
    }

    if (killpg(job->pid, SIGCONT) < 0)
    {
        perror("bg: failed to resume job");
        return;
    }

    free(job->state);
    job->state = strdup("Running");

    printf("[%d] %s &\n", job->job_number, job->command);
}
