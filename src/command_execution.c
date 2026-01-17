#define _POSIX_C_SOURCE 200809L
#include "command_execution.h"
#include "redirection.h"
#include "signals.h"
#include "job_control.h"
#include "activities.h"
#include "hop.h"
#include "reveal.h"
#include "log.h"
#include "ping.h"
#include "sequential_execution.h"
#include "background_execution.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <termios.h>

#define MAX_CMDS 64
#define MAX_ARGS 256

static char g_shell_cwd[4096] = "";
static char g_shell_home[4096] = "";
static char g_shell_previous[4096] = "";

void set_shell_state(char *current_cwd, char *home_directory, char *prev_directory)
{
    if (current_cwd) strncpy(g_shell_cwd, current_cwd, sizeof(g_shell_cwd) - 1);
    if (home_directory) strncpy(g_shell_home, home_directory, sizeof(g_shell_home) - 1);
    if (prev_directory) strncpy(g_shell_previous, prev_directory, sizeof(g_shell_previous) - 1);
    g_shell_cwd[sizeof(g_shell_cwd) - 1] = '\0';
    g_shell_home[sizeof(g_shell_home) - 1] = '\0';
    g_shell_previous[sizeof(g_shell_previous) - 1] = '\0';
}

void get_shell_state(char *current_cwd, char *home_directory, char *prev_directory)
{
    if (current_cwd) strcpy(current_cwd, g_shell_cwd);
    if (home_directory) strcpy(home_directory, g_shell_home);
    if (prev_directory) strcpy(prev_directory, g_shell_previous);
}

static int handle_builtin_command(char *cmd)
{
    char *first = strtok(strdup(cmd), " ");
    if (!first)
        return 0;

    int is_builtin = 0;

    if (strcmp(first, "reveal") == 0)
    {

        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)) == NULL)
        {
            perror("getcwd");
            free(first);
            return 1;
        }

        char *home_dir = getenv("HOME");
        if (!home_dir)
        {
            home_dir = ".";
        }

        char previous_dir[4096] = "";
        print_the_files(cmd, cwd, home_dir, previous_dir);
        is_builtin = 1;
    }
    else if (strcmp(first, "activities") == 0)
    {
        handle_activities_command();
        is_builtin = 1;
    }
    else if (strcmp(first, "ping") == 0)
    {
        char *args = cmd + 4; 
        while (*args == ' ' || *args == '\t')
            args++;
        handle_ping_command(*args ? args : NULL);
        is_builtin = 1;
    }
    else if (strcmp(first, "log") == 0)
    {
        char *args = cmd + 3;
        while (*args == ' ' || *args == '\t')
            args++;
        handle_log_command(*args ? args : NULL);
        is_builtin = 1;
    }

    free(first);
    return is_builtin;
}

void execute_command(char *input)
{
    if (!input)
        return;
    while (*input && (*input == ' ' || *input == '\t'))
        input++;
    if (*input == '\0')
        return;

    char saved_cwd[4096];
    if (getcwd(saved_cwd, sizeof(saved_cwd)) == NULL)
    {
        strcpy(saved_cwd, ".");
    }

    char *cmds[MAX_CMDS];
    int cmd_count = 0;
    char *copy = strdup(input);
    if (!copy)
        return;

    char *saveptr = NULL;
    char *part = strtok_r(copy, "|", &saveptr);
    while (part && cmd_count < MAX_CMDS)
    {
        while (*part == ' ' || *part == '\t')
            part++;
        char *end = part + strlen(part) - 1;
        while (end >= part && (*end == ' ' || *end == '\t' ||
                               *end == '\r' || *end == '\n'))
        {
            *end = '\0';
            end--;
        }
        if (*part)
        {
            cmds[cmd_count++] = strdup(part);
        }
        part = strtok_r(NULL, "|", &saveptr);
    }
    free(copy);

    if (cmd_count == 0)
        return;

    int pipefds[2 * (cmd_count - 1)];
    for (int i = 0; i < cmd_count - 1; ++i)
    {
        if (pipe(pipefds + i * 2) < 0)
        {
            perror("pipe");
            for (int j = 0; j < cmd_count; ++j)
                free(cmds[j]);
            return;
        }
    }

    pid_t pids[MAX_CMDS];
    for (int i = 0; i < cmd_count; ++i)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            pids[i] = -1;
            continue;
        }
        if (pid == 0)
        {
            if (i == 0)
                setpgid(0, 0);
            else
                setpgid(0, pids[0]);

            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);

            if (i > 0)
            {
                if (dup2(pipefds[(i - 1) * 2 + 0], STDIN_FILENO) < 0)
                {
                    perror("dup2 stdin");
                    _exit(1);
                }
            }
            if (i < cmd_count - 1)
            {
                if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) < 0)
                {
                    perror("dup2 stdout");
                    _exit(1);
                }
            }
            for (int j = 0; j < 2 * (cmd_count - 1); ++j)
                close(pipefds[j]);

            size_t seglen = strlen(cmds[i]);
            char *cleaned = malloc(seglen + 1024);
            if (!cleaned)
                _exit(1);
            cleaned[0] = '\0';
            if (handle_redirections(cmds[i], cleaned) < 0)
            {
                free(cleaned);
                _exit(1);
            }

            if (handle_builtin_command(cleaned))
            {
                free(cleaned);
                _exit(0);
            }

            char *argv[MAX_ARGS];
            int argc = 0;
            char *sav2 = NULL;
            char *tok = strtok_r(cleaned, " \t\r\n", &sav2);
            while (tok && argc < MAX_ARGS - 1)
            {
                argv[argc++] = tok;
                tok = strtok_r(NULL, " \t\r\n", &sav2);
            }
            argv[argc] = NULL;

            if (argc == 0)
            {
                free(cleaned);
                _exit(0);
            }

            if (handle_builtin_command(cleaned))
            {
                free(cleaned);
                _exit(0);
            }

            execvp(argv[0], argv);
            if (errno == ENOENT)
            {
                fprintf(stderr, "Command not found!\n");
            }
            else
            {
                perror("execvp");
            }
            free(cleaned);
            _exit(1);
        }
        else
        {
            if (i == 0)
                setpgid(pid, pid);
            else
                setpgid(pid, pids[0]);
            pids[i] = pid;

            add_to_activities(pid, cmds[i], 1);
        }
    }

    for (int j = 0; j < 2 * (cmd_count - 1); ++j)
        close(pipefds[j]);

    if (cmd_count > 0 && pids[0] > 0)
    {
        set_foreground_process_group(pids[0]);
        if (isatty(STDIN_FILENO))
        {
            tcsetpgrp(STDIN_FILENO, pids[0]);
        }
    }

    int pgid = (cmd_count > 0) ? pids[0] : -1;
    int alive = cmd_count;
    int stopped = 0;

    int waited_for[MAX_CMDS] = {0};

    while (alive > 0)
    {
        int status = 0;
        pid_t w = (pgid > 0) ? waitpid(-pgid, &status, WUNTRACED) : -1;
        if (w < 0)
        {
            if (errno == EINTR)
                continue;
            if (errno == ECHILD)
            {
                break;
            }
            break;
        }

        int found = 0;
        for (int i = 0; i < cmd_count; i++)
        {
            if (pids[i] == w && !waited_for[i])
            {
                waited_for[i] = 1;
                found = 1;
                break;
            }
        }

        if (WIFSTOPPED(status))
        {
            stopped = 1;
            update_process_state(w, "Stopped");
            break;
        }
        if (WIFEXITED(status) || WIFSIGNALED(status))
        {
            if (found)
            {
                alive--;
            }
            remove_from_activities(w);
        }
    }

    for (int i = 0; i < cmd_count; i++)
    {
        if (!waited_for[i] && pids[i] > 0)
        {
            int status;
            waitpid(pids[i], &status, WNOHANG);
        }
    }

    if (isatty(STDIN_FILENO))
    {
        tcsetpgrp(STDIN_FILENO, getpgrp());
    }
    clear_foreground_process_group();

    if (stopped && pgid > 0)
    {
        add_job(pgid, input);
        update_job_state(pgid, "Stopped");
        printf("Stopped\n");
        fflush(stdout);
    }

    for (int i = 0; i < cmd_count; ++i)
        free(cmds[i]);
}

void execute_shell_command(char *input)
{
    if (!input)
        return;

    while (*input && (*input == ' ' || *input == '\t'))
        input++;
    if (*input == '\0')
        return;

    char cwd[4096];
    char home_dir[4096];
    char previous_dir[4096];
    
    get_shell_state(cwd, home_dir, previous_dir);
    
    char current_cwd[4096];
    if (getcwd(current_cwd, sizeof(current_cwd)) != NULL)
    {
        strcpy(cwd, current_cwd);
    }

    char *input_copy = strdup(input);
    char *first = strtok(input_copy, " \t");
    if (first && strcmp(first, "hop") == 0)
    {
        check_first_word(input, cwd, home_dir, previous_dir);
      
        set_shell_state(cwd, home_dir, previous_dir);
        free(input_copy);
        return;
    }
    else if (first && strcmp(first, "reveal") == 0)
    {
        print_the_files(input, cwd, home_dir, previous_dir);
       
        set_shell_state(cwd, home_dir, previous_dir);
        free(input_copy);
        return;
    }
    else if (first && strcmp(first, "log") == 0)
    {
        char *args = input + 3;
        while (*args == ' ' || *args == '\t')
            args++;
        handle_log_command(*args ? args : NULL);
        free(input_copy);
        return;
    }
    else if (first && strcmp(first, "activities") == 0)
    {
        handle_activities_command();
        free(input_copy);
        return;
    }
    else if (first && strcmp(first, "ping") == 0)
    {
        char *args = input + 4;
        while (*args == ' ' || *args == '\t')
            args++;
        handle_ping_command(*args ? args : NULL);
        free(input_copy);
        return;
    }
    else if (first && strcmp(first, "fg") == 0)
    {
        char *args = input + 2;
        while (*args == ' ' || *args == '\t')
            args++;
        handle_fg_command(*args ? args : NULL);
        free(input_copy);
        return;
    }
    else if (first && strcmp(first, "bg") == 0)
    {
        char *args = input + 2;
        while (*args == ' ' || *args == '\t')
            args++;
        handle_bg_command(*args ? args : NULL);
        free(input_copy);
        return;
    }
    free(input_copy);

    // Handle complex commands with operators
    if (strchr(input, ';'))
    {
        execute_sequential(input);
    }
    else
    {
        char *input_copy2 = strdup(input);
        if (strip_background(input_copy2))
        {
            execute_background(input_copy2);
        }
        else
        {
            execute_command(input_copy2);
        }
        free(input_copy2);
    }
}
