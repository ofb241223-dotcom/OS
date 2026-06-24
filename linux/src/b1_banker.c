#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESS 10
#define MAX_RESOURCE 10

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
                printf("Error: P%d resource %d allocation exceeds maximum claim.\n", i, j);
                return 0;
            }
            used[j] += allocation[i][j];
        }
    }

    for (j = 0; j < n_resource; ++j) {
        if (used[j] > total[j]) {
            printf("Error: resource %d allocated amount exceeds total resources.\n", j);
            return 0;
        }
        available[j] = total[j] - used[j];
    }

    calculate_need();
    return 1;
}

void print_vector(const int data[]) {
    int j;

    printf("[");
    for (j = 0; j < n_resource; ++j) {
        printf("%d", data[j]);
        if (j != n_resource - 1) {
            printf(" ");
        }
    }
    printf("]");
}

void print_state(void) {
    int i, j;

    printf("\nCurrent system state\n");
    printf("Total     = ");
    print_vector(total);
    printf("\n");
    printf("Available = ");
    print_vector(available);
    printf("\n\n");

    printf("Process        Max        Allocation        Need\n");
    for (i = 0; i < n_process; ++i) {
        printf("P%-7d", i);
        print_vector(max_claim[i]);
        printf("     ");
        print_vector(allocation[i]);
        printf("        ");
        print_vector(need[i]);
        printf("\n");
    }
    printf("\n");
}

int safety_check(int sequence[]) {
    int work[MAX_RESOURCE];
    int finish[MAX_PROCESS] = {0};
    int i, j, count;
    int found;

    for (j = 0; j < n_resource; ++j) {
        work[j] = available[j];
    }

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
                for (j = 0; j < n_resource; ++j) {
                    work[j] += allocation[i][j];
                }
                finish[i] = 1;
                sequence[count++] = i;
                found = 1;
            }
        }

        if (!found) {
            return 0;
        }
    }

    return 1;
}

void print_safe_sequence(const int sequence[]) {
    int i;

    printf("Safe sequence: ");
    for (i = 0; i < n_process; ++i) {
        printf("P%d", sequence[i]);
        if (i != n_process - 1) {
            printf(" -> ");
        }
    }
    printf("\n");
}

void request_resources(void) {
    int pid;
    int request[MAX_RESOURCE];
    int sequence[MAX_PROCESS];
    int i;

    printf("Enter process id (0-%d): ", n_process - 1);
    if (scanf("%d", &pid) != 1) {
        return;
    }

    if (pid < 0 || pid >= n_process) {
        printf("Invalid process id.\n");
        return;
    }

    printf("Enter request vector (%d integers): ", n_resource);
    for (i = 0; i < n_resource; ++i) {
        scanf("%d", &request[i]);
    }

    for (i = 0; i < n_resource; ++i) {
        if (request[i] > need[pid][i]) {
            printf("Request exceeds process maximum remaining need. Request denied.\n");
            return;
        }
    }

    for (i = 0; i < n_resource; ++i) {
        if (request[i] > available[i]) {
            printf("Not enough available resources. Process P%d should wait.\n", pid);
            return;
        }
    }

    /* 先试探性分配，再用安全性算法检查。 */
    for (i = 0; i < n_resource; ++i) {
        available[i] -= request[i];
        allocation[pid][i] += request[i];
        need[pid][i] -= request[i];
    }

    if (safety_check(sequence)) {
        printf("Request granted. System remains in a safe state.\n");
        print_safe_sequence(sequence);
    } else {
        for (i = 0; i < n_resource; ++i) {
            available[i] += request[i];
            allocation[pid][i] -= request[i];
            need[pid][i] += request[i];
        }
        printf("Request denied. Trial allocation leads to an unsafe state.\n");
    }
}

void release_resources(void) {
    int pid;
    int release[MAX_RESOURCE];
    int i;

    printf("Enter process id (0-%d): ", n_process - 1);
    if (scanf("%d", &pid) != 1) {
        return;
    }

    if (pid < 0 || pid >= n_process) {
        printf("Invalid process id.\n");
        return;
    }

    printf("Enter release vector (%d integers): ", n_resource);
    for (i = 0; i < n_resource; ++i) {
        scanf("%d", &release[i]);
    }

    for (i = 0; i < n_resource; ++i) {
        if (release[i] > allocation[pid][i]) {
            printf("Release exceeds current allocation. Operation cancelled.\n");
            return;
        }
    }

    for (i = 0; i < n_resource; ++i) {
        allocation[pid][i] -= release[i];
        available[i] += release[i];
    }
    calculate_need();

    printf("Resources released successfully.\n");
}

void input_data(void) {
    int i, j;

    printf("Number of processes (<=%d): ", MAX_PROCESS);
    scanf("%d", &n_process);
    printf("Number of resource types (<=%d): ", MAX_RESOURCE);
    scanf("%d", &n_resource);

    if (n_process <= 0 || n_process > MAX_PROCESS || n_resource <= 0 || n_resource > MAX_RESOURCE) {
        printf("Input size out of range.\n");
        exit(1);
    }

    printf("Enter total resources vector (%d integers): ", n_resource);
    for (j = 0; j < n_resource; ++j) {
        scanf("%d", &total[j]);
    }

    printf("\nEnter Max matrix row by row.\n");
    for (i = 0; i < n_process; ++i) {
        printf("Max for P%d: ", i);
        for (j = 0; j < n_resource; ++j) {
            scanf("%d", &max_claim[i][j]);
        }
    }

    printf("\nEnter Allocation matrix row by row.\n");
    for (i = 0; i < n_process; ++i) {
        printf("Allocation for P%d: ", i);
        for (j = 0; j < n_resource; ++j) {
            scanf("%d", &allocation[i][j]);
        }
    }

    if (!check_initial_state()) {
        exit(1);
    }
}

int main(void) {
    int choice;
    int sequence[MAX_PROCESS];

    printf("===== Banker's Algorithm Demo =====\n");
    input_data();

    print_state();
    if (safety_check(sequence)) {
        printf("Initial state is SAFE.\n");
        print_safe_sequence(sequence);
    } else {
        printf("Initial state is UNSAFE.\n");
    }

    while (1) {
        printf("\nMenu\n");
        printf("1. Show current state\n");
        printf("2. Request resources\n");
        printf("3. Release resources\n");
        printf("4. Exit\n");
        printf("Choose: ");
        scanf("%d", &choice);

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
                printf("Program finished.\n");
                return 0;
            default:
                printf("Invalid choice.\n");
                break;
        }
    }
}
