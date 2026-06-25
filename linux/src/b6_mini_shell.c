#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 1024
#define MAX_ARGS 128
#define MAX_JOBS 128

typedef struct {
    pid_t pid;
    char command[MAX_LINE];
    int running;
} Job;

extern char **environ;

static Job jobs[MAX_JOBS];

void trim_newline(char *line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[len - 1] = '\0';
        len--;
    }
}

int is_blank_line(const char *line) {
    while (*line != '\0') {
        if (!isspace((unsigned char)*line)) {
            return 0;
        }
        line++;
    }
    return 1;
}

void add_job(pid_t pid, const char *command) {
    int i;

    for (i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].running = 1;
            strncpy(jobs[i].command, command, MAX_LINE - 1);
            jobs[i].command[MAX_LINE - 1] = '\0';
            return;
        }
    }
}

void mark_job_finished(pid_t pid) {
    int i;

    for (i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].pid == pid) {
            jobs[i].running = 0;
            return;
        }
    }
}

void remove_finished_jobs(void) {
    int i;

    for (i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].pid != 0 && jobs[i].running == 0) {
            jobs[i].pid = 0;
            jobs[i].command[0] = '\0';
        }
    }
}

void reap_background_jobs(void) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        const char *cmd = "unknown";
        int i;
        for (i = 0; i < MAX_JOBS; ++i) {
            if (jobs[i].pid == pid) {
                cmd = jobs[i].command;
                break;
            }
        }
        printf("\n[BG] Completed Job - PID: %d | Command: %s\n", pid, cmd);
        mark_job_finished(pid);
    }

    remove_finished_jobs();
}

void show_jobs(void) {
    int i;
    int found = 0;

    reap_background_jobs();
    for (i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].pid != 0) {
            if (!found) {
                printf("+--------+------------+-----------------------------------------------+\n");
                printf("| PID    | Status     | Command                                       |\n");
                printf("+--------+------------+-----------------------------------------------+\n");
            }
            printf("| %-6d | %-10s | %-45s |\n",
                   jobs[i].pid,
                   jobs[i].running ? "RUNNING" : "FINISHED",
                   jobs[i].command);
            found = 1;
        }
    }

    if (found) {
        printf("+--------+------------+-----------------------------------------------+\n");
    } else {
        printf("No active background jobs found.\n");
    }
}

void print_help(void) {
    printf("+----------------------+-----------------------------------------------+\n");
    printf("| Command              | Description                                   |\n");
    printf("+----------------------+-----------------------------------------------+\n");
    printf("| cd [dir]             | Change directory, or print current directory  |\n");
    printf("| environ              | Print all environment variables               |\n");
    printf("| echo [text]          | Print text to standard output                 |\n");
    printf("| jobs                 | List background child processes               |\n");
    printf("| help                 | Display this help dashboard                   |\n");
    printf("| quit / exit / bye    | Terminate the Mini Shell                      |\n");
    printf("+----------------------+-----------------------------------------------+\n");
    printf("| Note: Append '&' to run any external command in the background.      |\n");
    printf("+----------------------+-----------------------------------------------+\n");
}

void print_environ(void) {
    char **env = environ;

    while (*env != NULL) {
        printf("%s\n", *env);
        env++;
    }
}

int parse_line(char *line, char *args[], int *background) {
    int argc = 0;
    char *token;

    *background = 0;
    token = strtok(line, " \t");
    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc++] = token;
        token = strtok(NULL, " \t");
    }
    args[argc] = NULL;

    if (argc > 0 && strcmp(args[argc - 1], "&") == 0) {
        *background = 1;
        args[argc - 1] = NULL;
        argc--;
    }

    return argc;
}

int builtin_cd(char *args[], int argc) {
    char cwd[PATH_MAX];

    if (argc == 1) {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("getcwd");
        }
        return 1;
    }

    if (chdir(args[1]) != 0) {
        perror("cd");
        return 1;
    }

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        if (setenv("PWD", cwd, 1) != 0) {
            perror("setenv");
        }
    }

    return 1;
}

int builtin_echo(char *args[], int argc) {
    int i;

    for (i = 1; i < argc; ++i) {
        printf("%s", args[i]);
        if (i != argc - 1) {
            printf(" ");
        }
    }
    printf("\n");
    return 1;
}

int run_builtin(char *args[], int argc) {
    if (argc == 0) {
        return 1;
    }

    if (strcmp(args[0], "cd") == 0) {
        return builtin_cd(args, argc);
    }
    if (strcmp(args[0], "environ") == 0) {
        print_environ();
        return 1;
    }
    if (strcmp(args[0], "echo") == 0) {
        return builtin_echo(args, argc);
    }
    if (strcmp(args[0], "help") == 0) {
        print_help();
        return 1;
    }
    if (strcmp(args[0], "jobs") == 0) {
        show_jobs();
        return 1;
    }
    if (strcmp(args[0], "quit") == 0 || strcmp(args[0], "exit") == 0 ||
        strcmp(args[0], "bye") == 0) {
        return 0;
    }

    return -1;
}

void execute_external(char *args[], int background, const char *raw_command) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    }

    if (background) {
        add_job(pid, raw_command);
        printf("[BG] Started Job - PID: %d | Command: %s\n", pid, raw_command);
    } else {
        waitpid(pid, NULL, 0);
    }
}

int main(void) {
    char line[MAX_LINE];
    char raw_line[MAX_LINE];
    char *args[MAX_ARGS];

    printf("======================================================================\n");
    printf("                         MINI SHELL INTERPRETER\n");
    printf("======================================================================\n");
    printf("System initialized. Type 'help' to list available commands.\n");
    printf("======================================================================\n\n");

    while (1) {
        int argc;
        int background;
        int builtin_result;

        reap_background_jobs();
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("myshell:[%s]$ ", cwd);
        } else {
            printf("myshell>$ ");
        }
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        trim_newline(line);
        if (is_blank_line(line)) {
            continue;
        }

        strncpy(raw_line, line, sizeof(raw_line) - 1);
        raw_line[sizeof(raw_line) - 1] = '\0';

        argc = parse_line(line, args, &background);
        builtin_result = run_builtin(args, argc);

        if (builtin_result == 0) {
            break;
        }
        if (builtin_result == 1) {
            continue;
        }

        execute_external(args, background, raw_line);
    }

    return 0;
}
