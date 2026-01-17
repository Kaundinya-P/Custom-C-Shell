#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <termios.h>
#include "parser.h"
#include "hop.h"
#include "reveal.h"
#include "command_execution.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "redirection.h"
#include "sequential_execution.h"
#include "background_execution.h"
#include "log.h"
#include "activities.h"
#include "ping.h"
#include "job_control.h"
#include "signals.h"

#define MAX_TOKENS 256

static char *g_username = NULL;
static char *g_home_dir = NULL;
static struct utsname g_sysinfo;
char *get_current_prompt()
{
    static char prompt[8192];
    char cwd[4096];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        return NULL;
    }

    char display_path[4096];
    if (g_home_dir && strncmp(cwd, g_home_dir, strlen(g_home_dir)) == 0)
    {
        snprintf(display_path, sizeof(display_path), "~%s", cwd + strlen(g_home_dir));
        if (strcmp(display_path, "~") == 0)
            strcpy(display_path, "~");
    }
    else
    {
        strcpy(display_path, cwd);
    }

    snprintf(prompt, sizeof(prompt), "<%s@%s:%s> ",
             g_username ? g_username : "unknown",
             g_sysinfo.nodename,
             display_path);

    return prompt;
}
int main()
{
    char cwd[4096];
    char *username, *home_dir;
    struct utsname sysinfo;
    char input[1024];

    struct passwd *pw = getpwuid(getuid());
    if (pw)
        username = pw->pw_name;
    else
        username = "unknown";

    uname(&sysinfo);

    g_username = username;
    g_sysinfo = sysinfo;

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("getcwd");
        return 1;
    }
    home_dir = strdup(cwd);
    g_home_dir = home_dir;
    char previous_dir[4096];
    previous_dir[0] = '\0';
    
    // Initialize shared state for command execution
    set_shell_state(cwd, home_dir, previous_dir);

    init_log();

    setup_signal_handlers();

    while (1)
    {
        check_background_jobs();

        if (getcwd(cwd, sizeof(cwd)) == NULL)
        {
            perror("getcwd");
            return 1;
        }
        char display_path[4096];
        if (strncmp(cwd, home_dir, strlen(home_dir)) == 0)
        {
            snprintf(display_path, sizeof(display_path), "~%s", cwd + strlen(home_dir));
            if (strcmp(display_path, "~") == 0)
                strcpy(display_path, "~");
        }
        else
        {
            strcpy(display_path, cwd);
        }

        printf("<%s@%s:%s> ", username, sysinfo.nodename, display_path);
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            handle_eof();
        }

        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0)
            continue;

        add_to_log(input);

        Token tokens[MAX_TOKENS];
        size_t ntok = tokenize(input, tokens, MAX_TOKENS);
        if (!parse_shell_cmd(tokens, ntok))
        {
            fprintf(stderr, "Invalid Syntax!\n");

            for (size_t i = 0; i < ntok; i++)
            {
                if (tokens[i].type == TOK_NAME)
                    free(tokens[i].text);
            }
            continue;
        }

        char *input_copy = strdup(input);
        char *first = strtok(input_copy, " ");
        if (first && strcmp(first, "hop") == 0)
        {
            check_first_word(input, cwd, home_dir, previous_dir);
           
            set_shell_state(cwd, home_dir, previous_dir);
        }
        else if (first && strcmp(first, "reveal") == 0)
        {
            print_the_files(input, cwd, home_dir, previous_dir);
            
            set_shell_state(cwd, home_dir, previous_dir);
        }
        else if (first && strcmp(first, "log") == 0)
        {

            if(strchr(input, '<') || strchr(input, '>'))
            {
                execute_command(input);
            }
            else
            {
                char *args = input + 3;
                while (*args == ' ' || *args == '\t')
                    args++;
                handle_log_command(*args ? args : NULL);
            }
        }
        else if (first && strcmp(first, "activities") == 0)
        {
            handle_activities_command();
        }
        else if (first && strcmp(first, "ping") == 0)
        {

            char *args = input + 4;
            while (*args == ' ' || *args == '\t')
                args++;
            handle_ping_command(*args ? args : NULL);
        }
        else if (first && strcmp(first, "fg") == 0)
        {

            char *args = input + 2;
            while (*args == ' ' || *args == '\t')
                args++;
            handle_fg_command(*args ? args : NULL);
        }
        else if (first && strcmp(first, "bg") == 0)
        {

            char *args = input + 2;
            while (*args == ' ' || *args == '\t')
                args++;
            handle_bg_command(*args ? args : NULL);
        }
        else
        {
            if (strchr(input, ';'))
            {
                execute_sequential(input);
            }
            else
            {
                char *input_copy = strdup(input);
                if (strip_background(input_copy))
                {

                    execute_background(input_copy);
                }
                else
                {
                    execute_command(input_copy);
                }
                free(input_copy);
            }
        }
        if (input_copy)
            free(input_copy);

        for (size_t i = 0; i < ntok; i++)
        {
            if (tokens[i].type == TOK_NAME)
                free(tokens[i].text);
        }
    }

    free(home_dir);
    return 0;
}
