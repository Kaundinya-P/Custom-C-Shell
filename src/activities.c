#define _POSIX_C_SOURCE 200809L
#include "activities.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

typedef struct Process
{
    pid_t pid;
    char *command;
    char *state;    // "Running" / "Stopped"
    int foreground; // 1 = foreground, 0 = background
    struct Process *next;
} Process;

static Process *processes_head = NULL;


static char *get_process_state(pid_t pid)
{
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *file = fopen(path, "r");
    if (!file)
        return NULL;

    char state;
    if (fscanf(file, "%*d %*s %c", &state) != 1)
    {
        fclose(file);
        return NULL;
    }
    fclose(file);

    switch (state)
    {
    case 'R':
    case 'S':
    case 'D':
    case 'I':
        return strdup("Running");
    case 'T':
        return strdup("Stopped");
    case 'Z':
        return NULL; // Zombie → remove
    default:
        return strdup("Running");
    }
}

void add_to_activities(pid_t pid, const char *command, int foreground)
{
    Process *proc = malloc(sizeof(Process));
    if (!proc)
        return;

    proc->pid = pid;
    proc->command = strdup(command ? command : "unknown");
    proc->state = strdup("Running");
    proc->foreground = foreground;
    proc->next = processes_head;
    processes_head = proc;
}

void remove_from_activities(pid_t pid)
{
    Process *prev = NULL;
    Process *cur = processes_head;

    while (cur)
    {
        if (cur->pid == pid)
        {
            if (prev)
                prev->next = cur->next;
            else
                processes_head = cur->next;

            free(cur->command);
            free(cur->state);
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

void update_process_state(pid_t pid, const char *state)
{
    Process *cur = processes_head;
    while (cur)
    {
        if (cur->pid == pid)
        {
            free(cur->state);
            cur->state = strdup(state);
            return;
        }
        cur = cur->next;
    }
}

void handle_activities_command()
{
    // to handle dead processes and refresh states
    Process *prev = NULL;
    Process *cur = processes_head;

    while (cur)
    {
        int status;
        pid_t result = waitpid(cur->pid, &status, WNOHANG);

        if (result > 0)
        {

            Process *to_remove = cur;
            if (prev)
                prev->next = cur->next;
            else
                processes_head = cur->next;
            cur = cur->next;

            free(to_remove->command);
            free(to_remove->state);
            free(to_remove);
            continue;
        }
        else if (result == 0)
        {

            char *new_state = get_process_state(cur->pid);
            if (new_state)
            {
                free(cur->state);
                cur->state = new_state;
            }
            prev = cur;
            cur = cur->next;
        }
        else
        {

            prev = cur;
            cur = cur->next;
        }
    }

    if (!processes_head)
    {
        printf("No processes tracked.\n");
        return;
    }

    cur = processes_head;
    while (cur)
    {
        printf("[%d] : %s - %s\n",
               cur->pid,
               cur->command,
               cur->state);
        cur = cur->next;
    }
}
