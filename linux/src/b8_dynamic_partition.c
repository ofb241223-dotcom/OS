#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 1024
#define MAX_FREE 32
#define MAX_JOBS 32

typedef struct {
    int start;
    int size;
} FreeBlock;

typedef struct {
    int id;
    int start;
    int size;
    int active;
    int preset;
} Job;

enum {
    FIRST_FIT = 1,
    BEST_FIT = 2,
    WORST_FIT = 3
};

FreeBlock free_blocks[MAX_FREE];
Job jobs[MAX_JOBS];
int free_count = 0;
int strategy = FIRST_FIT;

const char *strategy_name(int s) {
    if (s == FIRST_FIT) {
        return "First Fit";
    }
    if (s == BEST_FIT) {
        return "Best Fit";
    }
    return "Worst Fit";
}

void sort_free_by_address(void) {
    int i, j;
    for (i = 0; i < free_count - 1; ++i) {
        for (j = i + 1; j < free_count; ++j) {
            if (free_blocks[i].start > free_blocks[j].start) {
                FreeBlock temp = free_blocks[i];
                free_blocks[i] = free_blocks[j];
                free_blocks[j] = temp;
            }
        }
    }
}

void merge_adjacent_free_blocks(void) {
    int i;

    sort_free_by_address();
    for (i = 0; i < free_count - 1;) {
        if (free_blocks[i].start + free_blocks[i].size == free_blocks[i + 1].start) {
            free_blocks[i].size += free_blocks[i + 1].size;
            memmove(&free_blocks[i + 1], &free_blocks[i + 2],
                    (free_count - i - 2) * sizeof(FreeBlock));
            free_count--;
        } else {
            i++;
        }
    }
}

void add_free_block(int start, int size) {
    free_blocks[free_count].start = start;
    free_blocks[free_count].size = size;
    free_count++;
    merge_adjacent_free_blocks();
}

void clear_jobs(void) {
    int i;
    for (i = 0; i < MAX_JOBS; ++i) {
        jobs[i].id = 0;
        jobs[i].start = 0;
        jobs[i].size = 0;
        jobs[i].active = 0;
        jobs[i].preset = 0;
    }
}

void set_job(int id, int start, int size, int preset) {
    int index = id - 1;
    jobs[index].id = id;
    jobs[index].start = start;
    jobs[index].size = size;
    jobs[index].active = 1;
    jobs[index].preset = preset;
}

void reset_system(void) {
    free_count = 0;
    clear_jobs();

    /* 初始空闲区表，便于观察后续分配与回收。 */
    add_free_block(80, 50);
    add_free_block(150, 100);
    add_free_block(600, 424);

    /* 题目中预设已存在的作业。 */
    set_job(2, 250, 200, 1);
    set_job(3, 450, 150, 1);
}

int select_free_block(int request_size) {
    int i;
    int index = -1;

    if (strategy == FIRST_FIT) {
        sort_free_by_address();
        for (i = 0; i < free_count; ++i) {
            if (free_blocks[i].size >= request_size) {
                return i;
            }
        }
        return -1;
    }

    if (strategy == BEST_FIT) {
        int best_size = 0x7fffffff;
        for (i = 0; i < free_count; ++i) {
            if (free_blocks[i].size >= request_size && free_blocks[i].size < best_size) {
                best_size = free_blocks[i].size;
                index = i;
            }
        }
        return index;
    }

    {
        int worst_size = -1;
        for (i = 0; i < free_count; ++i) {
            if (free_blocks[i].size >= request_size && free_blocks[i].size > worst_size) {
                worst_size = free_blocks[i].size;
                index = i;
            }
        }
    }
    return index;
}

void print_jobs(void) {
    int i;

    printf("Allocated jobs:\n");
    printf("%-8s %-10s %-10s %-10s\n", "Job", "Start", "Size", "Type");
    printf("%-8s %-10d %-10d %-10s\n", "System", 0, 80, "fixed");
    printf("%-8s %-10d %-10d %-10s\n", "Gap", 130, 20, "fixed");
    for (i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].active) {
            printf("Job%-4d %-10d %-10d %-10s\n",
                   jobs[i].id,
                   jobs[i].start,
                   jobs[i].size,
                   jobs[i].preset ? "preset" : "dynamic");
        }
    }
    printf("\n");
}

void print_free_blocks(void) {
    int i;
    FreeBlock temp[MAX_FREE];

    for (i = 0; i < free_count; ++i) {
        temp[i] = free_blocks[i];
    }

    if (strategy == FIRST_FIT) {
        sort_free_by_address();
    } else if (strategy == BEST_FIT) {
        int a, b;
        for (a = 0; a < free_count - 1; ++a) {
            for (b = a + 1; b < free_count; ++b) {
                if (free_blocks[a].size > free_blocks[b].size) {
                    FreeBlock swap = free_blocks[a];
                    free_blocks[a] = free_blocks[b];
                    free_blocks[b] = swap;
                }
            }
        }
    } else {
        int a, b;
        for (a = 0; a < free_count - 1; ++a) {
            for (b = a + 1; b < free_count; ++b) {
                if (free_blocks[a].size < free_blocks[b].size) {
                    FreeBlock swap = free_blocks[a];
                    free_blocks[a] = free_blocks[b];
                    free_blocks[b] = swap;
                }
            }
        }
    }

    printf("Free block table (%s order):\n", strategy_name(strategy));
    printf("%-8s %-10s %-10s\n", "Index", "Start", "Size");
    for (i = 0; i < free_count; ++i) {
        printf("%-8d %-10d %-10d\n", i + 1, free_blocks[i].start, free_blocks[i].size);
    }
    printf("\n");

    for (i = 0; i < free_count; ++i) {
        free_blocks[i] = temp[i];
    }
}

void show_state(void) {
    printf("\n========== Current State ==========\n");
    printf("Memory size: %d KB\n", MEMORY_SIZE);
    printf("Placement strategy: %s\n\n", strategy_name(strategy));
    print_jobs();
    print_free_blocks();
}

int allocate_job(int id, int size) {
    int block_index;
    int start;

    if (jobs[id - 1].active) {
        printf("Job %d already exists.\n", id);
        return 0;
    }

    block_index = select_free_block(size);
    if (block_index == -1) {
        printf("Job %d request %dKB failed: no suitable free block.\n", id, size);
        return 0;
    }

    start = free_blocks[block_index].start;
    set_job(id, start, size, 0);

    /* 采用低地址切割。 */
    free_blocks[block_index].start += size;
    free_blocks[block_index].size -= size;
    if (free_blocks[block_index].size == 0) {
        memmove(&free_blocks[block_index], &free_blocks[block_index + 1],
                (free_count - block_index - 1) * sizeof(FreeBlock));
        free_count--;
    }

    printf("Job %d allocated %dKB at address %d.\n", id, size, start);
    return 1;
}

void describe_neighbors(int start, int size) {
    int i;
    int has_upper = 0;
    int has_lower = 0;

    for (i = 0; i < free_count; ++i) {
        if (free_blocks[i].start + free_blocks[i].size == start) {
            has_upper = 1;
        }
        if (start + size == free_blocks[i].start) {
            has_lower = 1;
        }
    }

    if (has_upper && has_lower) {
        printf("Neighbor situation: upper and lower adjacent free blocks.\n");
    } else if (has_upper) {
        printf("Neighbor situation: only upper adjacent free block.\n");
    } else if (has_lower) {
        printf("Neighbor situation: only lower adjacent free block.\n");
    } else {
        printf("Neighbor situation: no adjacent free block.\n");
    }
}

int release_job(int id) {
    int index = id - 1;

    if (!jobs[index].active) {
        printf("Job %d is not allocated.\n", id);
        return 0;
    }

    printf("Releasing Job %d at address %d, size %dKB.\n",
           jobs[index].id,
           jobs[index].start,
           jobs[index].size);
    describe_neighbors(jobs[index].start, jobs[index].size);

    add_free_block(jobs[index].start, jobs[index].size);
    jobs[index].active = 0;
    jobs[index].preset = 0;
    return 1;
}

void run_fixed_sequence(void) {
    printf("\n===== Fixed experiment sequence begins =====\n");
    show_state();

    printf("Step 1: Job 1 requests 50KB.\n");
    allocate_job(1, 50);
    show_state();

    printf("Step 2: Release preset Job 2.\n");
    release_job(2);
    show_state();

    printf("Step 3: Release preset Job 3.\n");
    release_job(3);
    show_state();

    printf("Step 4: Job 4 requests 200KB.\n");
    allocate_job(4, 200);
    show_state();

    printf("Step 5: Release Job 1.\n");
    release_job(1);
    show_state();

    printf("Step 6: Release Job 4.\n");
    release_job(4);
    show_state();

    printf("Step 7: Job 5 requests 600KB.\n");
    allocate_job(5, 600);
    show_state();

    printf("===== Fixed experiment sequence ends =====\n");
}

void custom_allocate(void) {
    int id;
    int size;

    printf("Enter job id and size(KB): ");
    scanf("%d%d", &id, &size);
    allocate_job(id, size);
}

void custom_release(void) {
    int id;
    printf("Enter job id to release: ");
    scanf("%d", &id);
    release_job(id);
}

void set_strategy(void) {
    int s;
    printf("Choose strategy: 1.First Fit  2.Best Fit  3.Worst Fit : ");
    scanf("%d", &s);
    if (s >= 1 && s <= 3) {
        strategy = s;
        printf("Strategy changed to %s.\n", strategy_name(strategy));
    } else {
        printf("Invalid strategy.\n");
    }
}

int main(void) {
    int choice;

    reset_system();
    printf("===== Dynamic Partition Management Demo =====\n");

    while (1) {
        printf("\nMenu\n");
        printf("1. Show current state\n");
        printf("2. Set placement strategy\n");
        printf("3. Run fixed experiment sequence\n");
        printf("4. Allocate custom job\n");
        printf("5. Release custom job\n");
        printf("6. Reset system\n");
        printf("0. Exit\n");
        printf("Choose: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                show_state();
                break;
            case 2:
                set_strategy();
                break;
            case 3:
                run_fixed_sequence();
                break;
            case 4:
                custom_allocate();
                break;
            case 5:
                custom_release();
                break;
            case 6:
                reset_system();
                printf("System reset completed.\n");
                break;
            case 0:
                printf("Program finished.\n");
                return 0;
            default:
                printf("Invalid choice.\n");
                break;
        }
    }
}
