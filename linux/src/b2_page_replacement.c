#include <stdio.h>
#include <string.h>

#define MAX_PAGES 100
#define MAX_FRAMES 20
#define CELL_WIDTH 5

typedef struct {
    int frames[MAX_FRAMES];
    int marks[MAX_FRAMES];
    int fault;
} StepState;

typedef struct {
    const char *name;
    StepState steps[MAX_PAGES];
    int faults;
} SimulationResult;

int find_page(const int frames[], int frame_count, int page) {
    int i;
    for (i = 0; i < frame_count; ++i) {
        if (frames[i] == page) {
            return i;
        }
    }
    return -1;
}

void save_step(
    SimulationResult *result,
    int step_index,
    const int frames[],
    const int marks[],
    int frame_count,
    int fault
) {
    int i;

    result->steps[step_index].fault = fault;
    for (i = 0; i < frame_count; ++i) {
        result->steps[step_index].frames[i] = frames[i];
        result->steps[step_index].marks[i] = (marks == NULL) ? 0 : marks[i];
    }
}

void print_cell(int value, int marked) {
    char cell[16];

    if (value == -1) {
        strcpy(cell, "-");
    } else if (marked) {
        sprintf(cell, "%d*", value);
    } else {
        sprintf(cell, "%d", value);
    }

    printf("%-*s", CELL_WIDTH, cell);
}

void print_result(const SimulationResult *result, const int pages[], int page_count, int frame_count) {
    int row;
    int col;

    printf("\n%s\n", result->name);
    printf("Ref : ");
    for (col = 0; col < page_count; ++col) {
        printf("%-*d", CELL_WIDTH, pages[col]);
    }
    printf("\n");

    for (row = 0; row < frame_count; ++row) {
        printf("F%-3d: ", row + 1);
        for (col = 0; col < page_count; ++col) {
            print_cell(result->steps[col].frames[row], result->steps[col].marks[row]);
        }
        printf("\n");
    }

    printf("PF  : ");
    for (col = 0; col < page_count; ++col) {
        printf("%-*s", CELL_WIDTH, result->steps[col].fault ? "F" : "");
    }
    printf("\n");
    printf("%s page faults = %d\n", result->name, result->faults);
}

void simulate_fifo(const int pages[], int page_count, int frame_count, SimulationResult *result) {
    int frames[MAX_FRAMES];
    int marks[MAX_FRAMES];
    int pointer = 0;
    int i;

    result->name = "FIFO";
    result->faults = 0;

    for (i = 0; i < frame_count; ++i) {
        frames[i] = -1;
        marks[i] = 0;
    }

    for (i = 0; i < page_count; ++i) {
        if (find_page(frames, frame_count, pages[i]) == -1) {
            frames[pointer] = pages[i];
            pointer = (pointer + 1) % frame_count;
            result->faults++;
            save_step(result, i, frames, marks, frame_count, 1);
        } else {
            save_step(result, i, frames, marks, frame_count, 0);
        }
    }
}

void simulate_lru(const int pages[], int page_count, int frame_count, SimulationResult *result) {
    int frames[MAX_FRAMES];
    int marks[MAX_FRAMES];
    int last_used[MAX_FRAMES];
    int i;
    int j;
    int pos;

    result->name = "LRU";
    result->faults = 0;

    for (i = 0; i < frame_count; ++i) {
        frames[i] = -1;
        marks[i] = 0;
        last_used[i] = -1;
    }

    for (i = 0; i < page_count; ++i) {
        pos = find_page(frames, frame_count, pages[i]);
        if (pos != -1) {
            last_used[pos] = i;
            save_step(result, i, frames, marks, frame_count, 0);
            continue;
        }

        pos = -1;
        for (j = 0; j < frame_count; ++j) {
            if (frames[j] == -1) {
                pos = j;
                break;
            }
        }

        if (pos == -1) {
            int oldest = last_used[0];
            pos = 0;
            for (j = 1; j < frame_count; ++j) {
                if (last_used[j] < oldest) {
                    oldest = last_used[j];
                    pos = j;
                }
            }
        }

        frames[pos] = pages[i];
        last_used[pos] = i;
        result->faults++;
        save_step(result, i, frames, marks, frame_count, 1);
    }
}

void simulate_opt(const int pages[], int page_count, int frame_count, SimulationResult *result) {
    int frames[MAX_FRAMES];
    int marks[MAX_FRAMES];
    int i;
    int j;
    int pos;

    result->name = "OPT";
    result->faults = 0;

    for (i = 0; i < frame_count; ++i) {
        frames[i] = -1;
        marks[i] = 0;
    }

    for (i = 0; i < page_count; ++i) {
        pos = find_page(frames, frame_count, pages[i]);
        if (pos != -1) {
            save_step(result, i, frames, marks, frame_count, 0);
            continue;
        }

        pos = -1;
        for (j = 0; j < frame_count; ++j) {
            if (frames[j] == -1) {
                pos = j;
                break;
            }
        }

        if (pos == -1) {
            int farthest = -1;
            int replace_index = -1;

            for (j = 0; j < frame_count; ++j) {
                int k;
                int next_use = page_count + 1;

                for (k = i + 1; k < page_count; ++k) {
                    if (pages[k] == frames[j]) {
                        next_use = k;
                        break;
                    }
                }

                if (next_use > farthest) {
                    farthest = next_use;
                    replace_index = j;
                }
            }
            pos = replace_index;
        }

        frames[pos] = pages[i];
        result->faults++;
        save_step(result, i, frames, marks, frame_count, 1);
    }
}

void simulate_clock(const int pages[], int page_count, int frame_count, SimulationResult *result) {
    int frames[MAX_FRAMES];
    int ref_bit[MAX_FRAMES];
    int pointer = 0;
    int i;
    int pos;

    result->name = "CLOCK";
    result->faults = 0;

    for (i = 0; i < frame_count; ++i) {
        frames[i] = -1;
        ref_bit[i] = 0;
    }

    for (i = 0; i < page_count; ++i) {
        pos = find_page(frames, frame_count, pages[i]);
        if (pos != -1) {
            ref_bit[pos] = 1;
            save_step(result, i, frames, ref_bit, frame_count, 0);
            continue;
        }

        while (1) {
            if (frames[pointer] == -1) {
                break;
            }
            if (ref_bit[pointer] == 0) {
                break;
            }
            ref_bit[pointer] = 0;
            pointer = (pointer + 1) % frame_count;
        }

        frames[pointer] = pages[i];
        ref_bit[pointer] = 1;
        result->faults++;
        save_step(result, i, frames, ref_bit, frame_count, 1);
        pointer = (pointer + 1) % frame_count;
    }
}

int main(void) {
    int default_pages[] = {2, 3, 2, 1, 5, 2, 4, 5, 3, 2, 5, 2};
    int pages[MAX_PAGES];
    int page_count = 12;
    int frame_count = 3;
    int i;
    SimulationResult opt_result;
    SimulationResult lru_result;
    SimulationResult fifo_result;
    SimulationResult clock_result;

    for (i = 0; i < page_count; ++i) {
        pages[i] = default_pages[i];
    }

    simulate_opt(pages, page_count, frame_count, &opt_result);
    simulate_lru(pages, page_count, frame_count, &lru_result);
    simulate_fifo(pages, page_count, frame_count, &fifo_result);
    simulate_clock(pages, page_count, frame_count, &clock_result);

    print_result(&opt_result, pages, page_count, frame_count);
    print_result(&lru_result, pages, page_count, frame_count);
    print_result(&fifo_result, pages, page_count, frame_count);
    print_result(&clock_result, pages, page_count, frame_count);

    return 0;
}
