#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 10
#define MLFQ_LEVELS 3

enum {
    ALG_FCFS = 1,
    ALG_RR,
    ALG_SPN,
    ALG_SRT,
    ALG_HRRN,
    ALG_MLFQ
};

typedef struct {
    int id;
    int arrival_time;
    int service_time;
    int run_time;
    int remaining_time;
    char state;
    int finish_time;
    double turnaround_time;
    double weighted_turnaround_time;
    int queue_level;
    int quantum_used;
} PCB;

typedef struct {
    int process_count;
    PCB pcb[MAX_PROCESSES];
} ProcessSet;

typedef struct {
    int items[256];
    int front;
    int rear;
} Queue;

typedef struct {
    int pid;
    int start;
    int end;
} GanttSegment;

void queue_init(Queue *q) {
    q->front = 0;
    q->rear = 0;
}

int queue_empty(const Queue *q) {
    return q->front == q->rear;
}

void queue_push(Queue *q, int value) {
    q->items[q->rear++] = value;
}

int queue_pop(Queue *q) {
    return q->items[q->front++];
}

void copy_process_set(const ProcessSet *src, ProcessSet *dst) {
    memcpy(dst, src, sizeof(ProcessSet));
}

void initialize_runtime_fields(ProcessSet *set) {
    int i;

    for (i = 0; i < set->process_count; ++i) {
        set->pcb[i].run_time = 0;
        set->pcb[i].remaining_time = set->pcb[i].service_time;
        set->pcb[i].state = 'N';
        set->pcb[i].finish_time = -1;
        set->pcb[i].turnaround_time = 0.0;
        set->pcb[i].weighted_turnaround_time = 0.0;
        set->pcb[i].queue_level = 0;
        set->pcb[i].quantum_used = 0;
    }
}

int all_finished(const ProcessSet *set) {
    int i;

    for (i = 0; i < set->process_count; ++i) {
        if (set->pcb[i].state != 'F') {
            return 0;
        }
    }
    return 1;
}

void update_ready_states(ProcessSet *set, int current_time, int executing_index) {
    int i;

    for (i = 0; i < set->process_count; ++i) {
        if (set->pcb[i].state == 'F') {
            continue;
        }

        if (set->pcb[i].arrival_time > current_time) {
            set->pcb[i].state = 'N';
        } else if (i == executing_index) {
            set->pcb[i].state = 'E';
        } else {
            set->pcb[i].state = 'R';
        }
    }
}

void print_header(const char *name) {
    printf("\n============================================================\n");
    char title[128];
    sprintf(title, "PROCESS SCHEDULING: %s", name);
    int len = strlen(title);
    int pad = (60 - len) / 2;
    if (pad < 0) pad = 0;
    int i;
    for (i = 0; i < pad; ++i) printf(" ");
    printf("%s\n", title);
    printf("============================================================\n");
}

void print_time_slice_state(const ProcessSet *set, int current_time, int executing_index, int is_mlfq) {
    int i;

    printf("\n[Process Snapshot]\n");
    printf("Time: %d | Running: ", current_time);
    if (executing_index >= 0) {
        printf("P%d\n\n", set->pcb[executing_index].id);
    } else {
        printf("IDLE\n\n");
    }

    if (is_mlfq) {
        printf("+-----+---------+---------+-----+--------+-------+-------+\n");
        printf("| PID | Arrival | Service | Run | Remain | State | Level |\n");
        printf("+-----+---------+---------+-----+--------+-------+-------+\n");
        for (i = 0; i < set->process_count; ++i) {
            printf("| P%-2d | %-7d | %-7d | %-3d | %-6d | %-5c | %-5d |\n",
                   set->pcb[i].id,
                   set->pcb[i].arrival_time,
                   set->pcb[i].service_time,
                   set->pcb[i].run_time,
                   set->pcb[i].remaining_time,
                   set->pcb[i].state,
                   set->pcb[i].queue_level);
        }
        printf("+-----+---------+---------+-----+--------+-------+-------+\n");
    } else {
        printf("+-----+---------+---------+-----+--------+-------+\n");
        printf("| PID | Arrival | Service | Run | Remain | State |\n");
        printf("+-----+---------+---------+-----+--------+-------+\n");
        for (i = 0; i < set->process_count; ++i) {
            printf("| P%-2d | %-7d | %-7d | %-3d | %-6d | %-5c |\n",
                   set->pcb[i].id,
                   set->pcb[i].arrival_time,
                   set->pcb[i].service_time,
                   set->pcb[i].run_time,
                   set->pcb[i].remaining_time,
                   set->pcb[i].state);
        }
        printf("+-----+---------+---------+-----+--------+-------+\n");
    }
    printf("State: N=New  R=Ready  E=Executing  F=Finished\n");
}

void finalize_statistics(ProcessSet *set) {
    int i;
    for (i = 0; i < set->process_count; ++i) {
        set->pcb[i].turnaround_time = set->pcb[i].finish_time - set->pcb[i].arrival_time;
        set->pcb[i].weighted_turnaround_time =
            set->pcb[i].turnaround_time / set->pcb[i].service_time;
    }
}

int estimate_timeline_capacity(const ProcessSet *set) {
    int i;
    int total_service = 0;
    int max_arrival = 0;

    for (i = 0; i < set->process_count; ++i) {
        total_service += set->pcb[i].service_time;
        if (set->pcb[i].arrival_time > max_arrival) {
            max_arrival = set->pcb[i].arrival_time;
        }
    }

    if (total_service + max_arrival + 1 < 1) {
        return 1;
    }
    return total_service + max_arrival + 1;
}

int *allocate_history_buffer(const ProcessSet *set, int *capacity_out) {
    int capacity = estimate_timeline_capacity(set);
    int *history;
    int i;

    history = (int *)malloc(sizeof(int) * capacity);
    if (history == NULL) {
        return NULL;
    }

    for (i = 0; i < capacity; ++i) {
        history[i] = -1;
    }

    *capacity_out = capacity;
    return history;
}

void print_summary(const ProcessSet *set, const char *alg_name, const int *history, int total_time) {
    int i;
    double avg_turnaround = 0.0;
    double avg_weighted = 0.0;
    GanttSegment *segments = NULL;
    int segment_count = 0;

    printf("\n============================================================\n");
    char sum_title[128];
    sprintf(sum_title, "SUMMARY: %s", alg_name);
    int sum_len = strlen(sum_title);
    int sum_pad = (60 - sum_len) / 2;
    if (sum_pad < 0) sum_pad = 0;
    for (i = 0; i < sum_pad; ++i) printf(" ");
    printf("%s\n", sum_title);
    printf("============================================================\n");

    /* 1. 打印甘特图 */
    printf("\n[Gantt Chart]\n");

    if (total_time > 0) {
        segments = (GanttSegment *)malloc(sizeof(GanttSegment) * total_time);
        if (segments == NULL) {
            printf("  Failed to allocate Gantt chart buffer.\n");
        }
    }

    if (total_time > 0 && segments != NULL) {
        int last_pid = history[0];
        int start_time = 0;
        for (i = 1; i < total_time; ++i) {
            if (history[i] != last_pid) {
                segments[segment_count].pid = last_pid;
                segments[segment_count].start = start_time;
                segments[segment_count].end = i;
                segment_count++;
                start_time = i;
                last_pid = history[i];
            }
        }
        segments[segment_count].pid = last_pid;
        segments[segment_count].start = start_time;
        segments[segment_count].end = total_time;
        segment_count++;
    }

    if (segment_count > 0) {
        int total_width = 1;
        int pos = 0;
        char *line2;

        for (i = 0; i < segment_count; ++i) {
            int duration = segments[i].end - segments[i].start;
            int width = duration * 2;
            int min_width = (segments[i].pid == -1) ? 6 : 4;
            if (width < min_width) {
                width = min_width;
            }
            total_width += width + 1;
        }

        line2 = (char *)malloc(total_width + 16);
        if (line2 == NULL) {
            printf("  Failed to allocate Gantt chart scale buffer.\n");
        } else {
            memset(line2, ' ', total_width + 15);
            line2[total_width + 15] = '\0';
            memcpy(&line2[0], "0", 1);

            printf("  |");

            for (i = 0; i < segment_count; ++i) {
                int pid = segments[i].pid;
                int duration = segments[i].end - segments[i].start;
                int width = duration * 2;
                int min_width = (pid == -1) ? 6 : 4;
                char label[16];
                int label_len;
                int pad_left;
                int pad_right;
                char tick[16];
                int tick_len;
                int tick_pos;
                int j;

                if (width < min_width) {
                    width = min_width;
                }

                if (pid == -1) {
                    strcpy(label, "IDLE");
                } else {
                    sprintf(label, "P%d", pid);
                }

                label_len = strlen(label);
                pad_left = (width - label_len) / 2;
                pad_right = width - label_len - pad_left;

                for (j = 0; j < pad_left; ++j) printf(" ");
                printf("%s", label);
                for (j = 0; j < pad_right; ++j) printf(" ");
                printf("|");

                sprintf(tick, "%d", segments[i].end);
                tick_len = strlen(tick);
                tick_pos = pos + 1 + width - (tick_len - 1) / 2;
                memcpy(&line2[tick_pos], tick, tick_len);

                pos += 1 + width;
            }
            printf("\n");

            {
                int last_idx = total_width + 14;
                while (last_idx >= 0 && line2[last_idx] == ' ') {
                    last_idx--;
                }
                line2[last_idx + 1] = '\0';
            }

            printf("  %s\n", line2);
            free(line2);
        }
    } else {
        printf("  No execution segments.\n");
    }

    // 2. 进程性能指标
    printf("\n[Per-Process Metrics]\n");
    printf("+-----+--------+---------+------------+------------+\n");
    printf("| PID | Finish | Service | Turnaround | Weighted   |\n");
    printf("+-----+--------+---------+------------+------------+\n");
    for (i = 0; i < set->process_count; ++i) {
        printf("| P%-2d | %-6d | %-7d | %-10.2f | %-10.2f |\n",
               set->pcb[i].id,
               set->pcb[i].finish_time,
               set->pcb[i].service_time,
               set->pcb[i].turnaround_time,
               set->pcb[i].weighted_turnaround_time);

        avg_turnaround += set->pcb[i].turnaround_time;
        avg_weighted += set->pcb[i].weighted_turnaround_time;
    }
    printf("+-----+--------+---------+------------+------------+\n");

    // 3. 均值统计
    avg_turnaround /= set->process_count;
    avg_weighted /= set->process_count;

    printf("\n[Average Metrics]\n");
    printf("Average Turnaround Time         : %.2f\n", avg_turnaround);
    printf("Average Weighted Turnaround Time: %.2f\n", avg_weighted);
    printf("============================================================\n");

    free(segments);
}

int choose_fcfs(const ProcessSet *set, int current_time) {
    int i;
    int best = -1;

    for (i = 0; i < set->process_count; ++i) {
        if (set->pcb[i].arrival_time > current_time ||
            set->pcb[i].remaining_time <= 0 ||
            set->pcb[i].state == 'F') {
            continue;
        }

        if (best == -1 ||
            set->pcb[i].arrival_time < set->pcb[best].arrival_time ||
            (set->pcb[i].arrival_time == set->pcb[best].arrival_time &&
             set->pcb[i].id < set->pcb[best].id)) {
            best = i;
        }
    }
    return best;
}

int choose_spn(const ProcessSet *set, int current_time) {
    int i;
    int best = -1;

    for (i = 0; i < set->process_count; ++i) {
        if (set->pcb[i].arrival_time > current_time ||
            set->pcb[i].remaining_time <= 0 ||
            set->pcb[i].state == 'F') {
            continue;
        }

        if (best == -1 ||
            set->pcb[i].service_time < set->pcb[best].service_time ||
            (set->pcb[i].service_time == set->pcb[best].service_time &&
             set->pcb[i].arrival_time < set->pcb[best].arrival_time)) {
            best = i;
        }
    }
    return best;
}

int choose_srt(const ProcessSet *set, int current_time) {
    int i;
    int best = -1;

    for (i = 0; i < set->process_count; ++i) {
        if (set->pcb[i].arrival_time > current_time ||
            set->pcb[i].remaining_time <= 0 ||
            set->pcb[i].state == 'F') {
            continue;
        }

        if (best == -1 ||
            set->pcb[i].remaining_time < set->pcb[best].remaining_time ||
            (set->pcb[i].remaining_time == set->pcb[best].remaining_time &&
             set->pcb[i].arrival_time < set->pcb[best].arrival_time)) {
            best = i;
        }
    }
    return best;
}

int choose_hrrn(const ProcessSet *set, int current_time) {
    int i;
    int best = -1;
    double best_ratio = -1.0;

    for (i = 0; i < set->process_count; ++i) {
        double wait_time;
        double ratio;

        if (set->pcb[i].arrival_time > current_time ||
            set->pcb[i].remaining_time <= 0 ||
            set->pcb[i].state == 'F') {
            continue;
        }

        wait_time = current_time - set->pcb[i].arrival_time - set->pcb[i].run_time;
        ratio = (wait_time + set->pcb[i].service_time) / set->pcb[i].service_time;

        if (best == -1 || ratio > best_ratio) {
            best = i;
            best_ratio = ratio;
        }
    }
    return best;
}

void use_sample_data(ProcessSet *set) {
    int arrivals[] = {0, 1, 2, 3, 4};
    int services[] = {3, 5, 2, 6, 4};
    int i;

    set->process_count = 5;
    for (i = 0; i < set->process_count; ++i) {
        set->pcb[i].id = i;
        set->pcb[i].arrival_time = arrivals[i];
        set->pcb[i].service_time = services[i];
    }
}

int read_custom_data(ProcessSet *set) {
    int i;

    printf("Enter process count (1-%d): ", MAX_PROCESSES);
    if (scanf("%d", &set->process_count) != 1 ||
        set->process_count < 1 ||
        set->process_count > MAX_PROCESSES) {
        return 0;
    }

    for (i = 0; i < set->process_count; ++i) {
        set->pcb[i].id = i;
        printf("Process P%d arrival time and service time: ", i);
        if (scanf("%d %d", &set->pcb[i].arrival_time, &set->pcb[i].service_time) != 2 ||
            set->pcb[i].arrival_time < 0 ||
            set->pcb[i].service_time <= 0) {
            return 0;
        }
    }

    return 1;
}

void simulate_fcfs(ProcessSet *original) {
    ProcessSet set;
    int time = 0;
    int running = -1;
    int history_capacity = 0;
    int *history;

    copy_process_set(original, &set);
    initialize_runtime_fields(&set);
    history = allocate_history_buffer(&set, &history_capacity);
    if (history == NULL) {
        printf("Failed to allocate scheduling history buffer.\n");
        return;
    }
    print_header("FCFS");

    while (!all_finished(&set)) {
        if (running == -1 || set.pcb[running].remaining_time == 0) {
            running = choose_fcfs(&set, time);
        }

        update_ready_states(&set, time, running);
        print_time_slice_state(&set, time, running, 0);

        if (time < history_capacity) {
            history[time] = (running >= 0) ? set.pcb[running].id : -1;
        }

        if (running == -1) {
            time++;
            continue;
        }

        set.pcb[running].run_time++;
        set.pcb[running].remaining_time--;

        if (set.pcb[running].remaining_time == 0) {
            set.pcb[running].state = 'F';
            set.pcb[running].finish_time = time + 1;
            running = -1;
        }

        time++;
    }

    finalize_statistics(&set);
    print_summary(&set, "FCFS", history, time);
    free(history);
}

void simulate_rr(ProcessSet *original, int quantum) {
    ProcessSet set;
    Queue ready;
    int time = 0;
    int running = -1;
    int current_quantum = 0;
    int arrived[MAX_PROCESSES] = {0};
    int i;
    int history_capacity = 0;
    int *history;

    copy_process_set(original, &set);
    initialize_runtime_fields(&set);
    history = allocate_history_buffer(&set, &history_capacity);
    if (history == NULL) {
        printf("Failed to allocate scheduling history buffer.\n");
        return;
    }
    queue_init(&ready);
    print_header("RR");
    printf("Time quantum: %d\n", quantum);

    while (!all_finished(&set)) {
        for (i = 0; i < set.process_count; ++i) {
            if (!arrived[i] && set.pcb[i].arrival_time <= time) {
                queue_push(&ready, i);
                arrived[i] = 1;
            }
        }

        if (running == -1 && !queue_empty(&ready)) {
            running = queue_pop(&ready);
            current_quantum = 0;
        }

        update_ready_states(&set, time, running);
        print_time_slice_state(&set, time, running, 0);

        if (time < history_capacity) {
            history[time] = (running >= 0) ? set.pcb[running].id : -1;
        }

        if (running == -1) {
            time++;
            continue;
        }

        set.pcb[running].run_time++;
        set.pcb[running].remaining_time--;
        current_quantum++;

        if (set.pcb[running].remaining_time == 0) {
            set.pcb[running].state = 'F';
            set.pcb[running].finish_time = time + 1;
            running = -1;
            current_quantum = 0;
        } else if (current_quantum == quantum) {
            queue_push(&ready, running);
            running = -1;
            current_quantum = 0;
        }

        time++;
    }

    finalize_statistics(&set);
    print_summary(&set, "RR", history, time);
    free(history);
}

void simulate_spn(ProcessSet *original) {
    ProcessSet set;
    int time = 0;
    int running = -1;
    int history_capacity = 0;
    int *history;

    copy_process_set(original, &set);
    initialize_runtime_fields(&set);
    history = allocate_history_buffer(&set, &history_capacity);
    if (history == NULL) {
        printf("Failed to allocate scheduling history buffer.\n");
        return;
    }
    print_header("SPN");

    while (!all_finished(&set)) {
        if (running == -1) {
            running = choose_spn(&set, time);
        }

        update_ready_states(&set, time, running);
        print_time_slice_state(&set, time, running, 0);

        if (time < history_capacity) {
            history[time] = (running >= 0) ? set.pcb[running].id : -1;
        }

        if (running == -1) {
            time++;
            continue;
        }

        set.pcb[running].run_time++;
        set.pcb[running].remaining_time--;

        if (set.pcb[running].remaining_time == 0) {
            set.pcb[running].state = 'F';
            set.pcb[running].finish_time = time + 1;
            running = -1;
        }

        time++;
    }

    finalize_statistics(&set);
    print_summary(&set, "SPN", history, time);
    free(history);
}

void simulate_srt(ProcessSet *original) {
    ProcessSet set;
    int time = 0;
    int running = -1;
    int history_capacity = 0;
    int *history;

    copy_process_set(original, &set);
    initialize_runtime_fields(&set);
    history = allocate_history_buffer(&set, &history_capacity);
    if (history == NULL) {
        printf("Failed to allocate scheduling history buffer.\n");
        return;
    }
    print_header("SRT");

    while (!all_finished(&set)) {
        running = choose_srt(&set, time);
        update_ready_states(&set, time, running);
        print_time_slice_state(&set, time, running, 0);

        if (time < history_capacity) {
            history[time] = (running >= 0) ? set.pcb[running].id : -1;
        }

        if (running == -1) {
            time++;
            continue;
        }

        set.pcb[running].run_time++;
        set.pcb[running].remaining_time--;

        if (set.pcb[running].remaining_time == 0) {
            set.pcb[running].state = 'F';
            set.pcb[running].finish_time = time + 1;
        }

        time++;
    }

    finalize_statistics(&set);
    print_summary(&set, "SRT", history, time);
    free(history);
}

void simulate_hrrn(ProcessSet *original) {
    ProcessSet set;
    int time = 0;
    int running = -1;
    int history_capacity = 0;
    int *history;

    copy_process_set(original, &set);
    initialize_runtime_fields(&set);
    history = allocate_history_buffer(&set, &history_capacity);
    if (history == NULL) {
        printf("Failed to allocate scheduling history buffer.\n");
        return;
    }
    print_header("HRRN");

    while (!all_finished(&set)) {
        if (running == -1) {
            running = choose_hrrn(&set, time);
        }

        update_ready_states(&set, time, running);
        print_time_slice_state(&set, time, running, 0);

        if (time < history_capacity) {
            history[time] = (running >= 0) ? set.pcb[running].id : -1;
        }

        if (running == -1) {
            time++;
            continue;
        }

        set.pcb[running].run_time++;
        set.pcb[running].remaining_time--;

        if (set.pcb[running].remaining_time == 0) {
            set.pcb[running].state = 'F';
            set.pcb[running].finish_time = time + 1;
            running = -1;
        }

        time++;
    }

    finalize_statistics(&set);
    print_summary(&set, "HRRN", history, time);
    free(history);
}

void simulate_mlfq(ProcessSet *original) {
    ProcessSet set;
    Queue queues[MLFQ_LEVELS];
    int quantums[MLFQ_LEVELS] = {1, 2, 4};
    int arrived[MAX_PROCESSES] = {0};
    int time = 0;
    int running = -1;
    int i;
    int level;
    int history_capacity = 0;
    int *history;

    copy_process_set(original, &set);
    initialize_runtime_fields(&set);
    history = allocate_history_buffer(&set, &history_capacity);
    if (history == NULL) {
        printf("Failed to allocate scheduling history buffer.\n");
        return;
    }
    for (i = 0; i < MLFQ_LEVELS; ++i) {
        queue_init(&queues[i]);
    }
    print_header("MLFQ");
    printf("Queue quantums: 1, 2, 4\n");

    while (!all_finished(&set)) {
        for (i = 0; i < set.process_count; ++i) {
            if (!arrived[i] && set.pcb[i].arrival_time <= time) {
                set.pcb[i].queue_level = 0;
                set.pcb[i].quantum_used = 0;
                queue_push(&queues[0], i);
                arrived[i] = 1;
            }
        }

        if (running != -1) {
            for (level = 0; level < set.pcb[running].queue_level; ++level) {
                if (!queue_empty(&queues[level])) {
                    queue_push(&queues[set.pcb[running].queue_level], running);
                    set.pcb[running].quantum_used = 0;
                    running = -1;
                    break;
                }
            }
        }

        if (running == -1) {
            for (level = 0; level < MLFQ_LEVELS; ++level) {
                if (!queue_empty(&queues[level])) {
                    running = queue_pop(&queues[level]);
                    break;
                }
            }
        }

        update_ready_states(&set, time, running);
        print_time_slice_state(&set, time, running, 1);

        if (time < history_capacity) {
            history[time] = (running >= 0) ? set.pcb[running].id : -1;
        }

        if (running == -1) {
            time++;
            continue;
        }

        level = set.pcb[running].queue_level;
        set.pcb[running].run_time++;
        set.pcb[running].remaining_time--;
        set.pcb[running].quantum_used++;

        if (set.pcb[running].remaining_time == 0) {
            set.pcb[running].state = 'F';
            set.pcb[running].finish_time = time + 1;
            set.pcb[running].quantum_used = 0;
            running = -1;
        } else if (set.pcb[running].quantum_used >= quantums[level]) {
            if (set.pcb[running].queue_level < MLFQ_LEVELS - 1) {
                set.pcb[running].queue_level++;
            }
            set.pcb[running].quantum_used = 0;
            queue_push(&queues[set.pcb[running].queue_level], running);
            running = -1;
        }

        time++;
    }

    finalize_statistics(&set);
    print_summary(&set, "MLFQ", history, time);
    free(history);
}

void run_algorithm(ProcessSet *set, int algorithm) {
    switch (algorithm) {
        case ALG_FCFS:
            simulate_fcfs(set);
            break;
        case ALG_RR:
            simulate_rr(set, 1);
            break;
        case ALG_SPN:
            simulate_spn(set);
            break;
        case ALG_SRT:
            simulate_srt(set);
            break;
        case ALG_HRRN:
            simulate_hrrn(set);
            break;
        case ALG_MLFQ:
            simulate_mlfq(set);
            break;
        default:
            printf("Unknown algorithm.\n");
            break;
    }
}

void print_algorithm_menu(void) {
    printf("\nChoose algorithm:\n");
    printf("1. FCFS\n");
    printf("2. RR (time quantum = 1)\n");
    printf("3. SPN\n");
    printf("4. SRT\n");
    printf("5. HRRN\n");
    printf("6. MLFQ\n");
    printf("7. Run all algorithms\n");
    printf("Choose: ");
}

int main(void) {
    ProcessSet set;
    int data_choice;
    int algorithm_choice;

    printf("============================================================\n");
    printf("                  PROCESS SCHEDULING DEMO                   \n");
    printf("============================================================\n");
    printf("1. Use sample data\n");
    printf("2. Enter custom data\n");
    printf("Choose data source: ");

    if (scanf("%d", &data_choice) != 1) {
        printf("Invalid input.\n");
        return 1;
    }

    if (data_choice == 1) {
        use_sample_data(&set);
    } else if (data_choice == 2) {
        if (!read_custom_data(&set)) {
            printf("Invalid custom data.\n");
            return 1;
        }
    } else {
        printf("Unknown choice.\n");
        return 1;
    }

    print_algorithm_menu();
    if (scanf("%d", &algorithm_choice) != 1) {
        printf("Invalid input.\n");
        return 1;
    }

    if (algorithm_choice == 7) {
        run_algorithm(&set, ALG_FCFS);
        run_algorithm(&set, ALG_RR);
        run_algorithm(&set, ALG_SPN);
        run_algorithm(&set, ALG_SRT);
        run_algorithm(&set, ALG_HRRN);
        run_algorithm(&set, ALG_MLFQ);
    } else {
        run_algorithm(&set, algorithm_choice);
    }

    return 0;
}
