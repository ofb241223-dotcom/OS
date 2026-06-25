#include <stdio.h>
#include <string.h>

#define MAX_PAGES 100
#define MAX_FRAMES 20

// ANSI Color Codes (Redefined to empty string for monochrome output)
#define COLOR_RESET   ""
#define COLOR_BOLD    ""
#define COLOR_RED     ""
#define COLOR_GREEN   ""
#define COLOR_YELLOW  ""
#define COLOR_BLUE    ""
#define COLOR_MAGENTA ""
#define COLOR_CYAN    ""

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

void print_divider(int page_count) {
    int i;
    printf("+----------+");
    for (i = 0; i < page_count; ++i) {
        printf("---+");
    }
    printf("\n");
}

void print_result(const SimulationResult *result, const int pages[], int page_count, int frame_count) {
    int row, col;

    printf(COLOR_BLUE "\n------------------ %s Simulation ------------------\n" COLOR_RESET, result->name);
    
    print_divider(page_count);

    // Page Ref row
    printf("| Page Ref |");
    for (col = 0; col < page_count; ++col) {
        printf(" " COLOR_YELLOW "%d" COLOR_RESET " |", pages[col]);
    }
    printf("\n");

    print_divider(page_count);

    // Frame rows
    for (row = 0; row < frame_count; ++row) {
        printf("| Frame %-2d |", row + 1);
        for (col = 0; col < page_count; ++col) {
            int val = result->steps[col].frames[row];
            int marked = result->steps[col].marks[row];
            if (val == -1) {
                printf(" - |");
            } else {
                if (marked) {
                    printf(" " COLOR_CYAN "%d" COLOR_RESET COLOR_MAGENTA "*" COLOR_RESET "|", val);
                } else {
                    printf(" %d |", val);
                }
            }
        }
        printf("\n");
    }

    print_divider(page_count);

    // Fault? row
    printf("| Fault?   |");
    for (col = 0; col < page_count; ++col) {
        if (result->steps[col].fault) {
            printf(" " COLOR_RED "F" COLOR_RESET " |");
        } else {
            printf("   |");
        }
    }
    printf("\n");

    print_divider(page_count);

    printf("Simulation result: %s page faults = " COLOR_BOLD COLOR_RED "%d" COLOR_RESET "\n", result->name, result->faults);
}

void print_summary(const SimulationResult results[], int num_results, int page_count) {
    int i;
    printf(COLOR_CYAN "\n================== Page Replacement Algorithm Comparison ==================\n" COLOR_RESET);
    printf("+------------+------------------+-----------------+\n");
    printf("| Algorithm  | Page Faults (PF) | Page Fault Rate |\n");
    printf("+------------+------------------+-----------------+\n");
    for (i = 0; i < num_results; ++i) {
        double fault_rate = (double)results[i].faults / page_count * 100.0;
        printf("| %-10s |        " COLOR_BOLD COLOR_RED "%-2d" COLOR_RESET "        |      " COLOR_BOLD COLOR_RED "%5.2f%%" COLOR_RESET "      |\n",
               results[i].name, results[i].faults, fault_rate);
    }
    printf("+------------+------------------+-----------------+\n\n");
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
    int i, j, pos;

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
    int i, j, pos;

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
    int i, pos;

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
    SimulationResult results[4];

    for (i = 0; i < page_count; ++i) {
        pages[i] = default_pages[i];
    }

    simulate_opt(pages, page_count, frame_count, &results[0]);
    simulate_lru(pages, page_count, frame_count, &results[1]);
    simulate_fifo(pages, page_count, frame_count, &results[2]);
    simulate_clock(pages, page_count, frame_count, &results[3]);

    print_result(&results[0], pages, page_count, frame_count);
    print_result(&results[1], pages, page_count, frame_count);
    print_result(&results[2], pages, page_count, frame_count);
    print_result(&results[3], pages, page_count, frame_count);

    print_summary(results, 4, page_count);

    return 0;
}
