#define _POSIX_C_SOURCE 200809L
#include "signals.h"
#include "job_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static pid_t foreground_pgid = 0;
//  Signal handler for SIGCHLD to prevent zombie processes
static void sigchld_handler(int sig)
{
    (void)sig; // Suppress unused parameter warning

    // Reap any zombie children without blocking
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        // Child reaped - the background job checker will handle notifications
    }
}

// Signal handler for SIGINT (Ctrl-C)
static void sigint_handler(int sig)
{
    (void)sig; // Suppress unused parameter warning

    if (foreground_pgid > 0)
    {
        // Print newline after ^C
        // Send SIGINT to foreground process group
        if (killpg(foreground_pgid, SIGINT) == 0)
        {
            printf("\n");
            fflush(stdout);
        }
    }
    else
    {
        // No foreground process, just print newline
        printf("\n");
        fflush(stdout);
    }
}

// Signal handler for SIGTSTP (Ctrl-Z)
static void sigtstp_handler(int sig)
{
    (void)sig; // Suppress unused parameter warning

    if (foreground_pgid > 0)
    {
        // Send SIGTSTP to foreground process group
        if (killpg(foreground_pgid, SIGTSTP) == 0)
        {
            printf("\n");
            fflush(stdout);
            // Let the main loop handle the suspended job and return to prompt
            // The command execution loop will detect the stopped process
        }
    }
    else
    {
        // No foreground process, just print newline
        printf("\n");
        fflush(stdout);
    }
}

void setup_signal_handlers()
{
    struct sigaction sa;

    // Setup SIGINT handler (Ctrl-C)
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction SIGINT");
    }

    // Setup SIGTSTP handler (Ctrl-Z)
    sa.sa_handler = sigtstp_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGTSTP, &sa, NULL) == -1)
    {
        perror("sigaction SIGTSTP");
    }

    // Setup SIGCHLD handler to prevent zombie processes
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; // Don't deliver SIGCHLD for stopped children
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction SIGCHLD");
    }

    // Ignore SIGTTOU and SIGTTIN to prevent being stopped when trying to write to terminal
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
}
void set_foreground_process_group(pid_t pgid)
{
    foreground_pgid = pgid;
}

void clear_foreground_process_group()
{
    foreground_pgid = 0;
}

void handle_eof()
{
    // Clean exit on Ctrl+D
    printf("logout\n");
    fflush(stdout);
    exit(0);
}
