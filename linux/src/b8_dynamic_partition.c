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

typedef struct {
    int start;
    int size;
    int is_used;
    char owner[32];
} MemorySegment;

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

void print_memory_layout(void) {
    MemorySegment segments[128];
    int segment_count = 0;
    int i, j;

    // 1. 放入 System (fixed)
    segments[segment_count].start = 0;
    segments[segment_count].size = 80;
    segments[segment_count].is_used = 1;
    strcpy(segments[segment_count].owner, "System");
    segment_count++;

    // 2. 放入 Gap (fixed)
    segments[segment_count].start = 130;
    segments[segment_count].size = 20;
    segments[segment_count].is_used = 1;
    strcpy(segments[segment_count].owner, "Gap");
    segment_count++;

    // 3. 放入所有 active 的作业
    for (i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].active) {
            segments[segment_count].start = jobs[i].start;
            segments[segment_count].size = jobs[i].size;
            segments[segment_count].is_used = 1;
            sprintf(segments[segment_count].owner, "Job %d", jobs[i].id);
            segment_count++;
        }
    }

    // 4. 放入所有空闲分区
    for (i = 0; i < free_count; ++i) {
        segments[segment_count].start = free_blocks[i].start;
        segments[segment_count].size = free_blocks[i].size;
        segments[segment_count].is_used = 0;
        strcpy(segments[segment_count].owner, "-");
        segment_count++;
    }

    // 5. 排序
    for (i = 0; i < segment_count - 1; ++i) {
        for (j = i + 1; j < segment_count; ++j) {
            if (segments[i].start > segments[j].start) {
                MemorySegment temp = segments[i];
                segments[i] = segments[j];
                segments[j] = temp;
            }
        }
    }

    // 6. 打印表格
    printf("[Memory Layout]\n");
    printf("+---------+---------+---------+----------+-------------+\n");
    printf("| Start   | End     | Size    | Status   | Owner       |\n");
    printf("+---------+---------+---------+----------+-------------+\n");
    for (i = 0; i < segment_count; ++i) {
        int end = segments[i].start + segments[i].size - 1;
        char size_str[32];
        sprintf(size_str, "%d KB", segments[i].size);
        printf("| %-7d | %-7d | %-7s | %-8s | %-11s |\n",
               segments[i].start,
               end,
               size_str,
               segments[i].is_used ? "Used" : "Free",
               segments[i].owner);
    }
    printf("+---------+---------+---------+----------+-------------+\n");
}

void show_state(void) {
    print_memory_layout();
}

int allocate_job(int id, int size) {
    int block_index;
    int start;

    if (jobs[id - 1].active) {
        printf("[FAILED] Job %d already exists.\n", id);
        return 0;
    }

    block_index = select_free_block(size);
    if (block_index == -1) {
        printf("[FAILED] Job %d request %d KB failed: no suitable free block.\n", id, size);
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

    printf("[SUCCESS] Job %d allocated %d KB at address %d.\n", id, size, start);
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
        printf("[Neighbor] Upper and lower adjacent free blocks detected.\n");
    } else if (has_upper) {
        printf("[Neighbor] Only upper adjacent free block detected.\n");
    } else if (has_lower) {
        printf("[Neighbor] Only lower adjacent free block detected.\n");
    } else {
        printf("[Neighbor] No adjacent free block detected.\n");
    }
}

int release_job(int id) {
    int index = id - 1;

    if (!jobs[index].active) {
        printf("[FAILED] Job %d is not allocated.\n", id);
        return 0;
    }

    printf("[RELEASE] Releasing Job %d at address %d, size %d KB.\n",
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
    printf("Strategy: %s\n", strategy_name(strategy));

    printf("\n>>> [Current Step] Initial State\n");
    show_state();

    printf("\n>>> [Current Step] Step 1: Job 1 requests 50 KB\n");
    allocate_job(1, 50);
    show_state();

    printf("\n>>> [Current Step] Step 2: Release preset Job 2\n");
    release_job(2);
    show_state();

    printf("\n>>> [Current Step] Step 3: Release preset Job 3\n");
    release_job(3);
    show_state();

    printf("\n>>> [Current Step] Step 4: Job 4 requests 200 KB\n");
    allocate_job(4, 200);
    show_state();

    printf("\n>>> [Current Step] Step 5: Release Job 1\n");
    release_job(1);
    show_state();

    printf("\n>>> [Current Step] Step 6: Release Job 4\n");
    release_job(4);
    show_state();

    printf("\n>>> [Current Step] Step 7: Job 5 requests 600 KB\n");
    allocate_job(5, 600);
    show_state();
}

int main(void) {
    int choice;

    reset_system();
    printf("============================================================\n");
    printf("             DYNAMIC PARTITION MANAGEMENT SIMULATOR         \n");
    printf("============================================================\n");
    printf("Choose strategy:\n");
    printf("1. First Fit\n");
    printf("2. Best Fit\n");
    printf("3. Worst Fit\n");
    printf("Choose (1-3): ");
    
    if (scanf("%d", &choice) != 1) {
        return 0;
    }
    
    if (choice < 1 || choice > 3) {
        printf("Invalid choice. Exiting.\n");
        return 0;
    }
    
    strategy = choice;
    reset_system();
    run_fixed_sequence();
    
    return 0;
}
