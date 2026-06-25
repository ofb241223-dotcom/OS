#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERS 10
#define MAX_FILES 100
#define MAX_NAME_LEN 32
#define MAX_CONTENT_LEN 1024
#define MAX_LINE_LEN 1200

typedef struct {
    int used;
    char name[MAX_NAME_LEN];
} UserDir;

typedef struct {
    int used;
    char owner[MAX_NAME_LEN];
    char name[MAX_NAME_LEN];
    int physical_addr;
    int protect_code; /* 1:r 2:w 3:rw */
    int length;
    int is_open;
    int open_mode; /* 1:r 2:w 3:rw */
    char content[MAX_CONTENT_LEN];
} FileEntry;

UserDir user_dirs[MAX_USERS];
FileEntry files[MAX_FILES];
int next_physical_addr = 0;
char current_user[MAX_NAME_LEN] = "";
char open_file_name[MAX_NAME_LEN] = "";

int permission_from_text(const char *text) {
    if (strcmp(text, "r") == 0) {
        return 1;
    }
    if (strcmp(text, "w") == 0) {
        return 2;
    }
    if (strcmp(text, "rw") == 0 || strcmp(text, "wr") == 0) {
        return 3;
    }
    return 0;
}

const char *permission_to_text(int permission) {
    if (permission == 1) {
        return "r";
    }
    if (permission == 2) {
        return "w";
    }
    if (permission == 3) {
        return "rw";
    }
    return "-";
}

int find_user(const char *name) {
    int i;

    for (i = 0; i < MAX_USERS; ++i) {
        if (user_dirs[i].used && strcmp(user_dirs[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int create_user_if_needed(const char *name) {
    int i;
    int found = find_user(name);

    if (found != -1) {
        return found;
    }

    for (i = 0; i < MAX_USERS; ++i) {
        if (!user_dirs[i].used) {
            user_dirs[i].used = 1;
            strncpy(user_dirs[i].name, name, MAX_NAME_LEN - 1);
            user_dirs[i].name[MAX_NAME_LEN - 1] = '\0';
            return i;
        }
    }

    return -1;
}

int find_file_in_current_dir(const char *name) {
    int i;

    if (current_user[0] == '\0') {
        return -1;
    }

    for (i = 0; i < MAX_FILES; ++i) {
        if (files[i].used &&
            strcmp(files[i].owner, current_user) == 0 &&
            strcmp(files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void require_login(void) {
    if (current_user[0] == '\0') {
        printf("[ERROR] Please login first.\n");
    }
}

void cmd_help(void) {
    printf("[Help]\n");
    printf("+----------------------+------------------------------+\n");
    printf("| Command              | Description                  |\n");
    printf("+----------------------+------------------------------+\n");
    printf("| login <user>         | Log in or create user        |\n");
    printf("| logout               | Log out current user         |\n");
    printf("| dir                  | List directory               |\n");
    printf("| create <file> <mode> | Create file                  |\n");
    printf("| open <file> <mode>   | Open file                    |\n");
    printf("| close <file>         | Close file                   |\n");
    printf("| write <file> <text>  | Write file                   |\n");
    printf("| read <file>          | Read file                    |\n");
    printf("| delete <file>        | Delete file                  |\n");
    printf("| chmod <file> <mode>  | Change protection            |\n");
    printf("| pwd                  | Show current path            |\n");
    printf("| help                 | Show help                    |\n");
    printf("| exit / quit / bye    | Exit system                  |\n");
    printf("+----------------------+------------------------------+\n");
}

void cmd_pwd(void) {
    if (current_user[0] == '\0') {
        printf("/\n");
    } else {
        printf("/%s\n", current_user);
    }
}

void cmd_login(const char *user) {
    int index;

    if (user == NULL || user[0] == '\0') {
        printf("[ERROR] Usage: login <user>\n");
        return;
    }

    index = create_user_if_needed(user);
    if (index == -1) {
        printf("[ERROR] User directory table is full.\n");
        return;
    }

    strncpy(current_user, user, MAX_NAME_LEN - 1);
    current_user[MAX_NAME_LEN - 1] = '\0';
    open_file_name[0] = '\0';
    printf("[SUCCESS] Logged in as '%s'. Current directory: /%s\n", current_user, current_user);
}

void cmd_logout(void) {
    current_user[0] = '\0';
    open_file_name[0] = '\0';
    printf("[SUCCESS] Logged out.\n");
}

void cmd_dir(void) {
    int i;
    int found = 0;

    if (current_user[0] == '\0') {
        printf("[Main Directory]\n");
        printf("+----------------------+\n");
        printf("| Directory Name       |\n");
        printf("+----------------------+\n");
        for (i = 0; i < MAX_USERS; ++i) {
            if (user_dirs[i].used) {
                printf("| %-20s |\n", user_dirs[i].name);
                found = 1;
            }
        }
        if (!found) {
            printf("| %-20s |\n", "(empty)");
        }
        printf("+----------------------+\n");
        return;
    }

    printf("[Directory: /%s]\n", current_user);
    printf("+------------+----------+---------+--------+\n");
    printf("| Name       | Addr     | Protect | Length |\n");
    printf("+------------+----------+---------+--------+\n");
    for (i = 0; i < MAX_FILES; ++i) {
        if (files[i].used && strcmp(files[i].owner, current_user) == 0) {
            printf("| %-10s | %-8d | %-7s | %-6d |\n",
                   files[i].name,
                   files[i].physical_addr,
                   permission_to_text(files[i].protect_code),
                   files[i].length);
            found = 1;
        }
    }
    if (!found) {
        printf("| %-40s |\n", "(empty)");
    }
    printf("+------------+----------+---------+--------+\n");
}

void cmd_create(const char *name, const char *permission_text) {
    int i;
    int permission;

    if (current_user[0] == '\0') {
        require_login();
        return;
    }

    if (name == NULL || permission_text == NULL) {
        printf("[ERROR] Usage: create <file> <r|w|rw>\n");
        return;
    }

    if (find_file_in_current_dir(name) != -1) {
        printf("[ERROR] File already exists.\n");
        return;
    }

    permission = permission_from_text(permission_text);
    if (permission == 0) {
        printf("[ERROR] Invalid protection code.\n");
        return;
    }

    for (i = 0; i < MAX_FILES; ++i) {
        if (!files[i].used) {
            files[i].used = 1;
            strncpy(files[i].owner, current_user, MAX_NAME_LEN - 1);
            files[i].owner[MAX_NAME_LEN - 1] = '\0';
            strncpy(files[i].name, name, MAX_NAME_LEN - 1);
            files[i].name[MAX_NAME_LEN - 1] = '\0';
            files[i].physical_addr = next_physical_addr++;
            files[i].protect_code = permission;
            files[i].length = 0;
            files[i].is_open = 0;
            files[i].open_mode = 0;
            files[i].content[0] = '\0';
            printf("[SUCCESS] File created: %s (addr=%d, protect=%s)\n",
                   files[i].name,
                   files[i].physical_addr,
                   permission_to_text(files[i].protect_code));
            return;
        }
    }

    printf("[ERROR] File table is full.\n");
}

void cmd_open(const char *name, const char *mode_text) {
    int index;
    int mode;

    if (current_user[0] == '\0') {
        require_login();
        return;
    }

    if (name == NULL || mode_text == NULL) {
        printf("[ERROR] Usage: open <file> <r|w|rw>\n");
        return;
    }

    index = find_file_in_current_dir(name);
    if (index == -1) {
        printf("[ERROR] File not found.\n");
        return;
    }

    mode = permission_from_text(mode_text);
    if (mode == 0) {
        printf("[ERROR] Invalid open mode.\n");
        return;
    }

    if ((mode & files[index].protect_code) != mode) {
        printf("[ERROR] Open denied by protection code.\n");
        return;
    }

    files[index].is_open = 1;
    files[index].open_mode = mode;
    strncpy(open_file_name, name, MAX_NAME_LEN - 1);
    open_file_name[MAX_NAME_LEN - 1] = '\0';
    printf("[SUCCESS] File opened: %s (mode=%s)\n", name, permission_to_text(mode));
}

void cmd_close(const char *name) {
    int index;

    if (current_user[0] == '\0') {
        require_login();
        return;
    }

    if (name == NULL) {
        printf("[ERROR] Usage: close <file>\n");
        return;
    }

    index = find_file_in_current_dir(name);
    if (index == -1) {
        printf("[ERROR] File not found.\n");
        return;
    }

    files[index].is_open = 0;
    files[index].open_mode = 0;
    if (strcmp(open_file_name, name) == 0) {
        open_file_name[0] = '\0';
    }
    printf("[SUCCESS] File closed: %s\n", name);
}

void cmd_write(const char *name, const char *text) {
    int index;
    size_t len;

    if (current_user[0] == '\0') {
        require_login();
        return;
    }

    if (name == NULL || text == NULL) {
        printf("[ERROR] Usage: write <file> <text>\n");
        return;
    }

    index = find_file_in_current_dir(name);
    if (index == -1) {
        printf("[ERROR] File not found.\n");
        return;
    }

    if (!files[index].is_open) {
        printf("[ERROR] Write denied: file is not open.\n");
        return;
    }

    if ((files[index].open_mode & 2) == 0 || (files[index].protect_code & 2) == 0) {
        printf("[ERROR] Write denied by protection or open mode.\n");
        return;
    }

    strncpy(files[index].content, text, MAX_CONTENT_LEN - 1);
    files[index].content[MAX_CONTENT_LEN - 1] = '\0';
    len = strlen(files[index].content);
    files[index].length = (int)len;
    printf("[SUCCESS] Write completed. Length=%d\n", files[index].length);
}

void cmd_read(const char *name) {
    int index;

    if (current_user[0] == '\0') {
        require_login();
        return;
    }

    if (name == NULL) {
        printf("[ERROR] Usage: read <file>\n");
        return;
    }

    index = find_file_in_current_dir(name);
    if (index == -1) {
        printf("[ERROR] File not found.\n");
        return;
    }

    if (!files[index].is_open) {
        printf("[ERROR] Read denied: file is not open.\n");
        return;
    }

    if ((files[index].open_mode & 1) == 0 || (files[index].protect_code & 1) == 0) {
        printf("[ERROR] Read denied by protection or open mode.\n");
        return;
    }

    if (files[index].length == 0) {
        printf("[INFO] Content of %s: (empty)\n", name);
    } else {
        printf("[INFO] Content of %s: %s\n", name, files[index].content);
    }
}

void cmd_delete(const char *name) {
    int index;

    if (current_user[0] == '\0') {
        require_login();
        return;
    }

    if (name == NULL) {
        printf("[ERROR] Usage: delete <file>\n");
        return;
    }

    index = find_file_in_current_dir(name);
    if (index == -1) {
        printf("[ERROR] File not found.\n");
        return;
    }

    if (files[index].is_open) {
        printf("[ERROR] Delete denied: file is open.\n");
        return;
    }

    memset(&files[index], 0, sizeof(FileEntry));
    printf("[SUCCESS] File deleted: %s\n", name);
}

void cmd_chmod(const char *name, const char *permission_text) {
    int index;
    int permission;

    if (current_user[0] == '\0') {
        require_login();
        return;
    }

    if (name == NULL || permission_text == NULL) {
        printf("[ERROR] Usage: chmod <file> <r|w|rw>\n");
        return;
    }

    index = find_file_in_current_dir(name);
    if (index == -1) {
        printf("[ERROR] File not found.\n");
        return;
    }

    permission = permission_from_text(permission_text);
    if (permission == 0) {
        printf("[ERROR] Invalid protection code.\n");
        return;
    }

    files[index].protect_code = permission;
    printf("[SUCCESS] Protection updated: %s (protect=%s)\n", name, permission_to_text(permission));
}

void trim_newline(char *text) {
    size_t len = strlen(text);
    if (len > 0 && text[len - 1] == '\n') {
        text[len - 1] = '\0';
    }
}

int main(void) {
    char line[MAX_LINE_LEN];
    char line_copy[MAX_LINE_LEN];
    char write_copy[MAX_LINE_LEN];
    char *command;
    char *arg1;
    char *arg2;

    printf("============================================================\n");
    printf("                 SIMPLE TWO-LEVEL FILE SYSTEM               \n");
    printf("============================================================\n");
    printf("System initialized. Type 'help' to list available commands.\n");
    printf("============================================================\n");

    while (1) {
        if (current_user[0] == '\0') {
            printf("fs:/ > ");
        } else {
            printf("fs:/%s > ", current_user);
        }

        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        trim_newline(line);
        if (line[0] == '\0') {
            continue;
        }

        strncpy(line_copy, line, sizeof(line_copy) - 1);
        line_copy[sizeof(line_copy) - 1] = '\0';

        command = strtok(line, " ");
        arg1 = strtok(NULL, " ");
        arg2 = strtok(NULL, " ");

        if (strcmp(command, "help") == 0) {
            cmd_help();
        } else if (strcmp(command, "pwd") == 0) {
            cmd_pwd();
        } else if (strcmp(command, "login") == 0) {
            cmd_login(arg1);
        } else if (strcmp(command, "logout") == 0) {
            cmd_logout();
        } else if (strcmp(command, "dir") == 0) {
            cmd_dir();
        } else if (strcmp(command, "create") == 0) {
            cmd_create(arg1, arg2);
        } else if (strcmp(command, "open") == 0) {
            cmd_open(arg1, arg2);
        } else if (strcmp(command, "close") == 0) {
            cmd_close(arg1);
        } else if (strcmp(command, "write") == 0) {
            char *write_cmd;
            char *write_file;
            char *write_text;

            strncpy(write_copy, line_copy, sizeof(write_copy) - 1);
            write_copy[sizeof(write_copy) - 1] = '\0';

            write_cmd = strtok(write_copy, " ");
            write_file = strtok(NULL, " ");
            write_text = strtok(NULL, "");

            (void)write_cmd;

            if (write_file == NULL || write_text == NULL) {
                printf("[ERROR] Usage: write <file> <text>\n");
            } else {
                while (*write_text == ' ') {
                    write_text++;
                }

                if (*write_text == '\0') {
                    printf("[ERROR] Usage: write <file> <text>\n");
                } else {
                    cmd_write(write_file, write_text);
                }
            }
        } else if (strcmp(command, "read") == 0) {
            cmd_read(arg1);
        } else if (strcmp(command, "delete") == 0) {
            cmd_delete(arg1);
        } else if (strcmp(command, "chmod") == 0) {
            cmd_chmod(arg1, arg2);
        } else if (strcmp(command, "exit") == 0 ||
                   strcmp(command, "quit") == 0 ||
                   strcmp(command, "bye") == 0) {
            printf("[SUCCESS] File system halted.\n");
            break;
        } else {
            printf("[ERROR] Unknown command. Type 'help' for help.\n");
        }
        printf("\n");
    }

    return 0;
}
