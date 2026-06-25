#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESS 10
#define MAX_RESOURCE 10

// ANSI Color Codes (Redefined to empty string for monochrome output)
#define COLOR_RESET   ""
#define COLOR_BOLD    ""
#define COLOR_RED     ""
#define COLOR_GREEN   ""
#define COLOR_YELLOW  ""
#define COLOR_BLUE    ""
#define COLOR_MAGENTA ""
#define COLOR_CYAN    ""

int n_process;
int n_resource;
int total[MAX_RESOURCE];
int available[MAX_RESOURCE];
int max_claim[MAX_PROCESS][MAX_RESOURCE];
int allocation[MAX_PROCESS][MAX_RESOURCE];
int need[MAX_PROCESS][MAX_RESOURCE];

void calculate_need(void) {
    int i, j;
    for (i = 0; i < n_process; ++i) {
        for (j = 0; j < n_resource; ++j) {
            need[i][j] = max_claim[i][j] - allocation[i][j];
        }
    }
}

int check_initial_state(void) {
    int i, j;
    int used[MAX_RESOURCE] = {0};

    for (i = 0; i < n_process; ++i) {
        for (j = 0; j < n_resource; ++j) {
            if (allocation[i][j] > max_claim[i][j]) {
                printf(COLOR_RED "Error: P%d resource %c allocation exceeds maximum claim.\n" COLOR_RESET, i, 'A' + j);
                return 0;
            }
            used[j] += allocation[i][j];
        }
    }

    for (j = 0; j < n_resource; ++j) {
        if (used[j] > total[j]) {
            printf(COLOR_RED "Error: resource %c allocated amount (%d) exceeds total resources (%d).\n" COLOR_RESET, 'A' + j, used[j], total[j]);
            return 0;
        }
        available[j] = total[j] - used[j];
    }

    calculate_need();
    return 1;
}

void print_padded_string(const char *str, int width) {
    int len = strlen(str);
    int padding = width - len;
    int pad_left = padding / 2;
    int pad_right = padding - pad_left;
    int i;
    for (i = 0; i < pad_left; ++i) printf(" ");
    printf("%s", str);
    for (i = 0; i < pad_right; ++i) printf(" ");
}

void print_vector_centered(const int data[], int width) {
    int vec_width = 3 * n_resource + 3;
    int padding = width - vec_width;
    int pad_left = padding / 2;
    int pad_right = padding - pad_left;
    int i;
    for (i = 0; i < pad_left; ++i) printf(" ");
    
    printf("[ ");
    for (i = 0; i < n_resource; ++i) {
        printf("%2d ", data[i]);
    }
    printf("]");

    for (i = 0; i < pad_right; ++i) printf(" ");
}

void print_available_resources(void) {
    int j;
    printf(COLOR_YELLOW "[Available Resources] " COLOR_RESET);
    for (j = 0; j < n_resource; ++j) {
        printf(COLOR_BOLD "%c" COLOR_RESET ": %d", 'A' + j, available[j]);
        if (j != n_resource - 1) {
            printf("  ");
        }
    }
    printf("\n");
}

void print_state(void) {
    int i;
    int col_width = (3 * n_resource + 5 > 12) ? (3 * n_resource + 5) : 12;

    printf(COLOR_CYAN "\n======================================================================\n");
    printf("                     SYSTEM RESOURCE STATUS\n");
    printf("======================================================================\n" COLOR_RESET);
    
    print_available_resources();
    printf("\n");

    // Divider
    printf("+---------+");
    for (i = 0; i < col_width; ++i) printf("-");
    printf("+");
    for (i = 0; i < col_width; ++i) printf("-");
    printf("+");
    for (i = 0; i < col_width; ++i) printf("-");
    printf("+\n");

    // Headers
    printf("| Process |");
    print_padded_string("Max Claim", col_width);
    printf("|");
    print_padded_string("Allocation", col_width);
    printf("|");
    print_padded_string("Need", col_width);
    printf("|\n");

    // Divider
    printf("+---------+");
    for (i = 0; i < col_width; ++i) printf("-");
    printf("+");
    for (i = 0; i < col_width; ++i) printf("-");
    printf("+");
    for (i = 0; i < col_width; ++i) printf("-");
    printf("+\n");

    // Data rows
    for (i = 0; i < n_process; ++i) {
        printf("|   P%-2d   |", i);
        print_vector_centered(max_claim[i], col_width);
        printf("|");
        print_vector_centered(allocation[i], col_width);
        printf("|");
        print_vector_centered(need[i], col_width);
        printf("|\n");
    }

    // Divider
    printf("+---------+");
    for (i = 0; i < col_width; ++i) printf("-");
    printf("+");
    for (i = 0; i < col_width; ++i) printf("-");
    printf("+");
    for (i = 0; i < col_width; ++i) printf("-");
    printf("+\n\n");
}

void print_safety_divider(int col_width) {
    int i;
    printf("+------+---------+");
    for (i = 0; i < col_width; ++i) printf("-");
    printf("+");
    for (i = 0; i < col_width; ++i) printf("-");
    printf("+");
    for (i = 0; i < col_width; ++i) printf("-");
    printf("+");
    for (i = 0; i < col_width; ++i) printf("-");
    printf("+--------+\n");
}

void print_safe_sequence(const int sequence[]) {
    int i;
    printf(COLOR_GREEN "Safe sequence: " COLOR_RESET COLOR_BOLD COLOR_GREEN);
    for (i = 0; i < n_process; ++i) {
        printf("P%d", sequence[i]);
        if (i != n_process - 1) {
            printf(" -> ");
        }
    }
    printf(COLOR_RESET "\n");
}

int safety_check(int sequence[]) {
    int work[MAX_RESOURCE];
    int finish[MAX_PROCESS] = {0};
    int i, j, count;
    int found;
    int step = 1;
    int col_width = (3 * n_resource + 5 > 12) ? (3 * n_resource + 5) : 12;

    for (j = 0; j < n_resource; ++j) {
        work[j] = available[j];
    }

    printf(COLOR_CYAN "\nRunning safety algorithm check...\n" COLOR_RESET);
    print_safety_divider(col_width);
    printf("| Step | Process |");
    print_padded_string("Work", col_width);
    printf("|");
    print_padded_string("Need", col_width);
    printf("|");
    print_padded_string("Allocation", col_width);
    printf("|");
    print_padded_string("Work+Alloc", col_width);
    printf("| Finish |\n");
    print_safety_divider(col_width);

    count = 0;
    while (count < n_process) {
        found = 0;
        for (i = 0; i < n_process; ++i) {
            if (finish[i]) {
                continue;
            }

            for (j = 0; j < n_resource; ++j) {
                if (need[i][j] > work[j]) {
                    break;
                }
            }

            if (j == n_resource) {
                int next_work[MAX_RESOURCE];
                for (j = 0; j < n_resource; ++j) {
                    next_work[j] = work[j] + allocation[i][j];
                }

                printf("|  %-2d  |   P%-2d   |", step++, i);
                print_vector_centered(work, col_width);
                printf("|");
                print_vector_centered(need[i], col_width);
                printf("|");
                print_vector_centered(allocation[i], col_width);
                printf("|");
                print_vector_centered(next_work, col_width);
                printf("|  true  |\n");

                for (j = 0; j < n_resource; ++j) {
                    work[j] += allocation[i][j];
                }
                finish[i] = 1;
                sequence[count++] = i;
                found = 1;
                break; // Restart loop to check from P0 again
            }
        }

        if (!found) {
            print_safety_divider(col_width);
            printf(COLOR_RED "Safety check failed. System is in an UNSAFE state.\n" COLOR_RESET);
            return 0;
        }
    }

    print_safety_divider(col_width);
    printf(COLOR_GREEN "Safety check passed. System is in a SAFE state.\n" COLOR_RESET);
    return 1;
}

void request_resources(void) {
    int pid;
    int request[MAX_RESOURCE];
    int sequence[MAX_PROCESS];
    int i;

    printf(COLOR_CYAN "Enter process id (0-%d): " COLOR_RESET, n_process - 1);
    if (scanf("%d", &pid) != 1) {
        return;
    }

    if (pid < 0 || pid >= n_process) {
        printf(COLOR_RED "Error: Invalid process id.\n" COLOR_RESET);
        return;
    }

    printf(COLOR_CYAN "Enter request vector (%d integers): " COLOR_RESET, n_resource);
    for (i = 0; i < n_resource; ++i) {
        if (scanf("%d", &request[i]) != 1) {
            printf(COLOR_RED "Error: Invalid input.\n" COLOR_RESET);
            return;
        }
    }

    for (i = 0; i < n_resource; ++i) {
        if (request[i] > need[pid][i]) {
            printf(COLOR_RED "Request exceeds process maximum remaining need. Request denied.\n" COLOR_RESET);
            return;
        }
    }

    for (i = 0; i < n_resource; ++i) {
        if (request[i] > available[i]) {
            printf(COLOR_YELLOW "Not enough available resources. Process P%d should wait.\n" COLOR_RESET, pid);
            return;
        }
    }

    /* Trial allocation */
    for (i = 0; i < n_resource; ++i) {
        available[i] -= request[i];
        allocation[pid][i] += request[i];
        need[pid][i] -= request[i];
    }

    if (safety_check(sequence)) {
        printf(COLOR_GREEN "Request granted. System remains in a safe state.\n" COLOR_RESET);
        print_safe_sequence(sequence);
    } else {
        // Rollback
        for (i = 0; i < n_resource; ++i) {
            available[i] += request[i];
            allocation[pid][i] -= request[i];
            need[pid][i] += request[i];
        }
        printf(COLOR_RED "Request denied. Trial allocation leads to an unsafe state.\n" COLOR_RESET);
    }
}

void release_resources(void) {
    int pid;
    int release[MAX_RESOURCE];
    int i;

    printf(COLOR_CYAN "Enter process id (0-%d): " COLOR_RESET, n_process - 1);
    if (scanf("%d", &pid) != 1) {
        return;
    }

    if (pid < 0 || pid >= n_process) {
        printf(COLOR_RED "Error: Invalid process id.\n" COLOR_RESET);
        return;
    }

    printf(COLOR_CYAN "Enter release vector (%d integers): " COLOR_RESET, n_resource);
    for (i = 0; i < n_resource; ++i) {
        if (scanf("%d", &release[i]) != 1) {
            printf(COLOR_RED "Error: Invalid input.\n" COLOR_RESET);
            return;
        }
    }

    for (i = 0; i < n_resource; ++i) {
        if (release[i] > allocation[pid][i]) {
            printf(COLOR_RED "Release exceeds current allocation. Operation cancelled.\n" COLOR_RESET);
            return;
        }
    }

    for (i = 0; i < n_resource; ++i) {
        allocation[pid][i] -= release[i];
        available[i] += release[i];
    }
    calculate_need();

    printf(COLOR_GREEN "Resources released successfully.\n" COLOR_RESET);
}

void input_data(void) {
    int i, j;

    printf(COLOR_CYAN "Number of processes (<=%d): " COLOR_RESET, MAX_PROCESS);
    if (scanf("%d", &n_process) != 1) exit(1);
    printf(COLOR_CYAN "Number of resource types (<=%d): " COLOR_RESET, MAX_RESOURCE);
    if (scanf("%d", &n_resource) != 1) exit(1);

    if (n_process <= 0 || n_process > MAX_PROCESS || n_resource <= 0 || n_resource > MAX_RESOURCE) {
        printf(COLOR_RED "Input size out of range.\n" COLOR_RESET);
        exit(1);
    }

    printf(COLOR_CYAN "Enter total resources vector (%d integers): " COLOR_RESET, n_resource);
    for (j = 0; j < n_resource; ++j) {
        if (scanf("%d", &total[j]) != 1) exit(1);
    }

    printf(COLOR_CYAN "\nEnter Max matrix row by row.\n" COLOR_RESET);
    for (i = 0; i < n_process; ++i) {
        printf("Max for P%d: ", i);
        for (j = 0; j < n_resource; ++j) {
            if (scanf("%d", &max_claim[i][j]) != 1) exit(1);
        }
    }

    printf(COLOR_CYAN "\nEnter Allocation matrix row by row.\n" COLOR_RESET);
    for (i = 0; i < n_process; ++i) {
        printf("Allocation for P%d: ", i);
        for (j = 0; j < n_resource; ++j) {
            if (scanf("%d", &allocation[i][j]) != 1) exit(1);
        }
    }

    if (!check_initial_state()) {
        exit(1);
    }
}

int main(void) {
    int choice;
    int sequence[MAX_PROCESS];

    printf(COLOR_CYAN "======================================================================\n");
    printf("                     BANKER'S ALGORITHM SIMULATOR\n");
    printf("======================================================================\n" COLOR_RESET);
    input_data();

    print_state();
    if (safety_check(sequence)) {
        printf(COLOR_GREEN "\nInitial state is SAFE.\n" COLOR_RESET);
        print_safe_sequence(sequence);
    } else {
        printf(COLOR_RED "\nInitial state is UNSAFE.\n" COLOR_RESET);
    }

    while (1) {
        printf(COLOR_BLUE "\n--- System Menu ---\n" COLOR_RESET);
        printf("1. Show current system state\n");
        printf("2. Request resources for process\n");
        printf("3. Release resources from process\n");
        printf("4. Exit simulator\n");
        printf(COLOR_CYAN "Choose an option: " COLOR_RESET);
        if (scanf("%d", &choice) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            printf(COLOR_RED "Invalid choice. Please enter a number.\n" COLOR_RESET);
            continue;
        }

        switch (choice) {
            case 1:
                print_state();
                break;
            case 2:
                request_resources();
                break;
            case 3:
                release_resources();
                break;
            case 4:
                printf(COLOR_YELLOW "Program finished.\n" COLOR_RESET);
                return 0;
            default:
                printf(COLOR_RED "Invalid choice.\n" COLOR_RESET);
                break;
        }
    }
}
