#define _POSIX_C_SOURCE 200809L
#include "background_execution.h"
#include "command_execution.h"
#include "activities.h"
#include "job_control.h"
#include "shell_prompt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef struct BgJob
{
    int job_number;
    pid_t pid;
    char *command;
    struct BgJob *next;
} BgJob;

static BgJob *jobs_head = NULL;
static int next_job_num = 1;

static void add_job_entry(pid_t pid, const char *cmd)
{
    BgJob *j = malloc(sizeof(BgJob));
    if (!j)
        return;
    j->job_number = next_job_num++;
    j->pid = pid;
    j->command = strdup(cmd ? cmd : "");
    j->next = jobs_head;
    jobs_head = j;
    printf("[%d] %d\n", j->job_number, (int)j->pid);
    fflush(stdout);
}

void execute_background(char *cmd)
{
    if (!cmd)
        return;
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return;
    }

    if (pid == 0)
    {
        if (setpgid(0, 0) < 0)
        {
            // do nothing here
        }

        int devnull = open("/dev/null", O_RDONLY);
        if (devnull >= 0)
        {
            dup2(devnull, STDIN_FILENO);
            close(devnull);
        }

        execute_command(cmd);

        _exit(EXIT_SUCCESS);
    }
    else
    {

        add_job_entry(pid, cmd);

        add_to_activities(pid, cmd, 0);
        add_job(pid, cmd);

    }
}

int check_background_jobs(void)
{
    BgJob *prev = NULL;
    BgJob *cur = jobs_head;
    int jobs_completed = 0;

    while (cur)
    {
        int status;
        pid_t res = waitpid(cur->pid, &status, WNOHANG);
        if (res == 0)
        {

            prev = cur;
            cur = cur->next;
            continue;
        }
        else if (res < 0)
        {

            prev = cur;
            cur = cur->next;
            continue;
        }
        else
        {
            jobs_completed = 1;
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            {
                printf("%s with pid %d exited normally\n", cur->command, (int)cur->pid);
            }
            else
            {
                printf("%s with pid %d exited abnormally\n", cur->command, (int)cur->pid);
            }

            char *prompt = get_current_prompt();
            if (prompt)
            {
                printf("%s", prompt);
            }
            fflush(stdout);

            remove_from_activities(cur->pid);
            remove_job(cur->pid);

            BgJob *tofree = cur;
            if (prev)
                prev->next = cur->next;
            else
                jobs_head = cur->next;
            cur = cur->next;
            free(tofree->command);
            free(tofree);
            continue;
        }
    }

    return jobs_completed;
}

int strip_background(char *input)
{
    if (!input)
        return 0;
    size_t len = strlen(input);
    while (len > 0 && (input[len - 1] == ' ' || input[len - 1] == '\t' ||
                       input[len - 1] == '\r' || input[len - 1] == '\n'))
    {
        input[--len] = '\0';
    }
    if (len > 0 && input[len - 1] == '&')
    {
        input[--len] = '\0';
        while (len > 0 && (input[len - 1] == ' ' || input[len - 1] == '\t'))
        {
            input[--len] = '\0';
        }
        return 1;
    }
    return 0;
}
