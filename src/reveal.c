#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "reveal.h"
#include <unistd.h>
#include <libgen.h>

int cmpstring(const void *a, const void *b)
{
    const char *sa = *(const char *const *)a;
    const char *sb = *(const char *const *)b;
    return strcmp(sa, sb);
}

int print_the_files(const char *input, char *dir_name, char *home_dir, char *previous_dir)
{
    char buf[1024];
    strncpy(buf, input, sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    char *first = strtok(buf, " \t\r\n");
    if (first && strcmp(first, "reveal") == 0)
    {
        int show_all = 0;
        int line_mode = 0;

        char *arg = strtok(NULL, " \t\r\n");

        while (arg && arg[0] == '-' && strlen(arg) > 1)
        {
            for (int i = 1; arg[i]; i++)
            {
                if (arg[i] == 'a')
                    show_all = 1;
                else if (arg[i] == 'l')
                    line_mode = 1;
                else
                {
                    printf("reveal: Invalid Syntax!\n");
                    return 0;
                }
            }
            arg = strtok(NULL, " \t\r\n");
        }

        char *second = arg;

        if (second)
        {
            char *third = strtok(NULL, " \t\r\n");
            if (third)
            {
                printf("reveal: Invalid Syntax!\n");
                return 0;
            }
        }
        if (second && strcmp(second, "..") == 0)
        {
            char temp[4096];
            strncpy(temp, dir_name, sizeof(temp));
            temp[sizeof(temp) - 1] = '\0';
            char *parent = dirname(temp);
            struct dirent *de;
            DIR *dr = opendir(parent);

            if (dr == NULL)
            {
                printf("No such directory!\n");
                return 0;
            }

            char *names[1024];
            int count = 0;
            while ((de = readdir(dr)) != NULL)
            {
                if (!show_all && de->d_name[0] == '.')
                    continue;
                names[count] = strdup(de->d_name);
                count++;
            }
            closedir(dr);

            qsort(names, count, sizeof(char *), cmpstring);
            for (int i = 0; i < count; i++)
            {
                if (line_mode)
                    printf("%s\n", names[i]);
                else
                    printf("%s  ", names[i]);
                free(names[i]);
            }
            if (!line_mode)
                printf("\n");
        }

        else if (second && strcmp(second, "~") == 0)
        {
            struct dirent *de;
            DIR *dr = opendir(home_dir);

            if (dr == NULL)
            {
                printf("No such directory!\n");
                return 0;
            }

            char *names[1024];
            int count = 0;
            while ((de = readdir(dr)) != NULL)
            {
                if (!show_all && de->d_name[0] == '.')
                    continue;
                names[count] = strdup(de->d_name);
                count++;
            }
            closedir(dr);

            qsort(names, count, sizeof(char *), cmpstring);
            for (int i = 0; i < count; i++)
            {
                if (line_mode)
                    printf("%s\n", names[i]);
                else
                    printf("%s  ", names[i]);
                free(names[i]);
            }
            if (!line_mode)
                printf("\n");
        }

        else if (second && strcmp(second, "-") == 0)
        {
            if (previous_dir[0] != '\0')
            {
                char swap[4096];
                strcpy(swap, dir_name);
                struct dirent *de;
                DIR *dr = opendir(previous_dir);

                if (dr == NULL)
                {
                    printf("No such directory!\n");
                    return 0;
                }

                char *names[1024];
                int count = 0;
                while ((de = readdir(dr)) != NULL)
                {
                    if (!show_all && de->d_name[0] == '.')
                        continue;
                    names[count] = strdup(de->d_name);
                    count++;
                }
                closedir(dr);

                qsort(names, count, sizeof(char *), cmpstring);
                for (int i = 0; i < count; i++)
                {
                    if (line_mode)
                        printf("%s\n", names[i]);
                    else
                        printf("%s  ", names[i]);
                    free(names[i]);
                }
                if (!line_mode)
                    printf("\n");
                strcpy(previous_dir, swap);
            }
            else
            {
                printf("No such directory!\n");
            }
        }
        else if (second == NULL)
        {
            struct dirent *de;
            DIR *dr = opendir(dir_name);

            if (dr == NULL)
            {
                printf("No such directory!\n");
                return 0;
            }

            char *names[1024];
            int count = 0;
            while ((de = readdir(dr)) != NULL)
            {
                if (!show_all && de->d_name[0] == '.')
                    continue;
                names[count] = strdup(de->d_name);
                count++;
            }
            closedir(dr);

            qsort(names, count, sizeof(char *), cmpstring);
            for (int i = 0; i < count; i++)
            {
                if (line_mode)
                    printf("%s\n", names[i]);
                else
                    printf("%s  ", names[i]);
                free(names[i]);
            }
            if (!line_mode)
                printf("\n");
        }

        else
        {
            struct dirent *de;

            char path[4096];
            if (second[0] == '/')
                strncpy(path, second, sizeof(path));
            else
                snprintf(path, sizeof(path), "%s/%s", dir_name, second);

            DIR *dr = opendir(path);

            if (dr == NULL)
            {
                printf("No such directory!\n");
                return 0;
            }

            char *names[1024];
            int count = 0;
            while ((de = readdir(dr)) != NULL)
            {
                if (!show_all && de->d_name[0] == '.')
                    continue;
                names[count] = strdup(de->d_name);
                count++;
            }
            closedir(dr);

            qsort(names, count, sizeof(char *), cmpstring);
            for (int i = 0; i < count; i++)
            {
                if (line_mode)
                    printf("%s\n", names[i]);
                else
                    printf("%s  ", names[i]);
                free(names[i]);
            }
            if (!line_mode)
                printf("\n");
        }
    }

    return 0;
}
