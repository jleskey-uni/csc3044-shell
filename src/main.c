// Author: Joseph Leskey
// Date: 21 April 2025

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <sys/wait.h>

#define ARG_LIST_LENGTH 9

bool streq(const char *a, const char *b);

char *getUsername();
void writeWorkingDirectory(bool relative, char *target, size_t size);

int parseCommand(char argv[][ARG_LIST_LENGTH], bool *background);
void runProcess(int argc, char argv[][ARG_LIST_LENGTH], const bool background);
void changeDirectory(int argc, char argv[][ARG_LIST_LENGTH]);
bool processCommand();
bool prompt();

int main()
{
    while (prompt())
        ;
}

bool prompt()
{
    char curDir[NAME_MAX];
    writeWorkingDirectory(true, curDir, NAME_MAX);

    printf("%s (%s) %% ", getUsername(), curDir);
    fflush(stdout);

    return processCommand();
}

bool processCommand()
{
    char argv[PATH_MAX][ARG_LIST_LENGTH];
    bool background = false;
    int argc = parseCommand(argv, &background);

    if (argc)
    {
        char *command = argv[0];

        if (streq(command, "exit"))
        {
            return false;
        }
        else if (streq(command, "cd"))
        {
            changeDirectory(argc, argv);
        }
        else if (streq(command, "clear"))
        {
            system("clear");
        }
        else
        {
            runProcess(argc, argv, background);
        }
    }

    return true;
}

void changeDirectory(int argc, char argv[][ARG_LIST_LENGTH])
{
    char path[PATH_MAX] = "";

    if (argc > 1)
    {
        snprintf(path, sizeof(path), "%s", argv[1]);
    }
    else
    {
        const char *home = getenv("HOME");

        if (home != NULL)
        {
            snprintf(path, sizeof(path), "%s", home);
        }
        else
        {
            fprintf(stderr, "Home directory not found\n");
        }
    }

    if (chdir(path) != 0)
    {
        perror("cd");
    }
}

void runProcess(int argc, char argv[][ARG_LIST_LENGTH], bool background)
{
    char *command = argv[0];
    pid_t pid = fork();

    if (pid == 0)
    {
        // Child process

        // Make sure we have our null termination.
        char *argv_ptrs[ARG_LIST_LENGTH + 1];
        for (int i = 0; i < argc; i++)
        {
            argv_ptrs[i] = argv[i];
        }
        argv_ptrs[argc] = NULL;

        execvp(command, argv_ptrs);

        perror(command);
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        // Parent process
        if (!background)
        {
            waitpid(pid, NULL, 0);
        }
        else
        {
            printf("Running in background (PID %d)\n", pid);
        }
    }
    else
    {
        fprintf(stderr, "Failed to fork\n");
    }
}

int parseCommand(char argv[][ARG_LIST_LENGTH], bool *background)
{
    // Buffer to read into from command line.
    char line[PATH_MAX * ARG_LIST_LENGTH];
    int argc = 0;

    if (fgets(line, sizeof(line), stdin))
    {
        char token[PATH_MAX];
        int tokenLength = 0;

        char *c = line;

        bool quoted = false;
        bool escaped = false;
        *background = false;

        while (*c)
        {
            if (*background && *c != '&' && !isspace(*c))
            {
                fprintf(stderr, "Unexpected token '%c' after '&'\n", *c);
            }
            else if (!escaped && quoted && *c == '"')
            {
                // End quote.
                quoted = false;
            }
            else if (!escaped && *c == '"')
            {
                // Start quote.
                quoted = true;
            }
            else if (!escaped && *c == '\\')
            {
                escaped = true;
            }
            else if (!quoted && !escaped && *c == '&')
            {
                *background = true;
            }
            else if (quoted || escaped || !isspace(*c))
            {
                // Reset escaped status.
                escaped = false;

                if (tokenLength < PATH_MAX - 1)
                {
                    token[tokenLength++] = *c;
                }
            }
            else
            {
                // Must be a space or EOL.
                if (tokenLength)
                {
                    if (argc < ARG_LIST_LENGTH)
                    {
                        token[tokenLength++] = '\0';
                        strncpy(argv[argc++], token, PATH_MAX);
                        tokenLength = 0;
                    }
                    else
                    {
                        fprintf(stderr, "Too many arguments\n");
                        return 0;
                    }
                }
            }

            c++;
        }
    }
    else
    {
        printf("^D\n");
        fprintf(stderr, "To exit, type `exit` or press Ctrl + C.\n");
        clearerr(stdin);
    }

    return argc;
}

char *getUsername()
{
    // We'll avoid relying on the USER environment variable.
    struct passwd *passwordEntry = getpwuid(getuid());

    if (passwordEntry != NULL)
    {
        return passwordEntry->pw_name;
    }

    fprintf(stderr, "No username.\n");
    exit(EXIT_FAILURE);
}

void writeWorkingDirectory(bool relative, char *target, size_t size)
{
    char path[PATH_MAX];

    if (getcwd(path, PATH_MAX) != NULL)
    {
        if (relative)
        {
            char *name = strrchr(path, '/');

            if (name != NULL)
                name++;

            snprintf(target, size, "%s", name);
        }
        else
        {
            snprintf(target, size, "%s", path);
        }
    }
    else
    {
        fprintf(stderr, "No working directory.\n");
        exit(EXIT_FAILURE);
    }
}

bool streq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}
