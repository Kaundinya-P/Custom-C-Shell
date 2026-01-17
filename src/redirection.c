#include "redirection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int handle_redirections(const char *input, char *clean_cmd)
{
    char *copy = strdup(input);
    if (!copy)
    {
        perror("strdup");
        return -1;
    }

    char *all_infiles[1024];
    char *all_outfiles[1024];
    int all_append_modes[1024];
    int input_count = 0, output_count = 0;

    char *src = copy;
    char *dst = clean_cmd;

    while (*src)
    {
        if (*src == '<')
        {
            // part of input redirection
            src++;
            while (*src == ' ' || *src == '\t')
                src++;
            char *filename_start = src;
            while (*src && *src != ' ' && *src != '\t' &&
                   *src != '<' && *src != '>' && *src != '|' &&
                   *src != ';' && *src != '&')
            {
                src++;
            }
            if (*src)
                *src++ = '\0';
            if (input_count < 1024)
            {
                all_infiles[input_count++] = filename_start;
            }
        }
        else if (*src == '>')
        {
            // for output redirection
            src++;
            int is_append = 0;
            if (*src == '>')
            {
                is_append = 1;
                src++;
            }

            while (*src == ' ' || *src == '\t')
                src++;
            char *filename_start = src;
            while (*src && *src != ' ' && *src != '\t' &&
                   *src != '<' && *src != '>' && *src != '|' &&
                   *src != ';' && *src != '&')
            {
                src++;
            }
            if (*src)
                *src++ = '\0';
            if (output_count < 1024)
            {
                all_outfiles[output_count] = filename_start;
                all_append_modes[output_count] = is_append;
                output_count++;
            }
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';

    dst--;
    while (dst >= clean_cmd && (*dst == ' ' || *dst == '\t'))
        *dst-- = '\0';

    if (input_count > 0)
    {
        int final_input_fd = -1;
        for (int i = 0; i < input_count; i++)
        {
            int fd = open(all_infiles[i], O_RDONLY);
            if (fd < 0)
            {
                fprintf(stderr, "No such file or directory\n");
                free(copy);
                return -1;
            }
            if (final_input_fd >= 0)
            {
                close(final_input_fd);
            }
            final_input_fd = fd;
        }
        if (final_input_fd >= 0)
        {
            if (dup2(final_input_fd, STDIN_FILENO) < 0)
            {
                perror("dup2 input");
                close(final_input_fd);
                free(copy);
                return -1;
            }
            close(final_input_fd);
        }
    }
    if (output_count > 0)
    {
        int final_output_fd = -1;
        for (int i = 0; i < output_count; i++)
        {
            int fd;
            if (all_append_modes[i])
                fd = open(all_outfiles[i], O_WRONLY | O_CREAT | O_APPEND, 0644);
            else
                fd = open(all_outfiles[i], O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if (fd < 0)
            {
                fprintf(stderr, "Unable to create file for writing\n");
                if (final_output_fd >= 0)
                    close(final_output_fd);
                free(copy);
                return -1;
            }
            if (final_output_fd >= 0)
            {
                close(final_output_fd); // Close previous fd
            }
            final_output_fd = fd;
        }
        if (final_output_fd >= 0)
        {
            if (dup2(final_output_fd, STDOUT_FILENO) < 0)
            {
                perror("dup2 output");
                close(final_output_fd);
                free(copy);
                return -1;
            }
            close(final_output_fd);
        }
    }

    free(copy);
    return 0;
}
