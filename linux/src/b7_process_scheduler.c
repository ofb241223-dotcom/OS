#include <stdio.h>
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
    printf("Algorithm: %s\n", name);
    printf("============================================================\n");
}

void print_time_slice_state(const ProcessSet *set, int current_time, int executing_index) {
    int i;

    printf("\nTime %d\n", current_time);
    printf("Running: ");
    if (executing_index >= 0) {
        printf("P%d\n", set->pcb[executing_index].id);
    } else {
        printf("IDLE\n");
    }

    printf("%-4s %-7s %-7s %-7s %-9s %-7s %-7s\n",
           "PID",
           "Arrive",
           "Serve",
           "Run",
           "Remain",
           "State",
           "Level");

    for (i = 0; i < set->process_count; ++i) {
        printf("P%-3d %-7d %-7d %-7d %-9d %-7c %-7d\n",
               set->pcb[i].id,
               set->pcb[i].arrival_time,
               set->pcb[i].service_time,
               set->pcb[i].run_time,
               set->pcb[i].remaining_time,
               set->pcb[i].state,
               set->pcb[i].queue_level);
    }
}

void finalize_statistics(ProcessSet *set) {
    int i;

    for (i = 0; i < set->process_count; ++i) {
        set->pcb[i].turnaround_time = set->pcb[i].finish_time - set->pcb[i].arrival_time;
        set->pcb[i].weighted_turnaround_time =
            set->pcb[i].turnaround_time / set->pcb[i].service_time;
    }
}

void print_summary(const ProcessSet *set) {
    int i;
    double avg_turnaround = 0.0;
    double avg_weighted = 0.0;

    printf("\nSummary\n");
    printf("%-4s %-8s %-8s %-12s %-12s\n",
           "PID",
           "Finish",
           "Service",
           "Turnaround",
           "Weighted");

    for (i = 0; i < set->process_count; ++i) {
        printf("P%-3d %-8d %-8d %-12.2f %-12.2f\n",
               set->pcb[i].id,
               set->pcb[i].finish_time,
               set->pcb[i].service_time,
               set->pcb[i].turnaround_time,
               set->pcb[i].weighted_turnaround_time);

        avg_turnaround += set->pcb[i].turnaround_time;
        avg_weighted += set->pcb[i].weighted_turnaround_time;
    }

    avg_turnaround /= set->process_count;
    avg_weighted /= set->process_count;

    printf("Average turnaround time: %.2f\n", avg_turnaround);
    printf("Average weighted turnaround time: %.2f\n", avg_weighted);
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

    copy_process_set(original, &set);
    initialize_runtime_fields(&set);
    print_header("FCFS");

    while (!all_finished(&set)) {
        if (running == -1 || set.pcb[running].remaining_time == 0) {
            running = choose_fcfs(&set, time);
        }

        update_ready_states(&set, time, running);
        print_time_slice_state(&set, time, running);

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
    print_summary(&set);
}

void simulate_rr(ProcessSet *original, int quantum) {
    ProcessSet set;
    Queue ready;
    int time = 0;
    int running = -1;
    int current_quantum = 0;
    int arrived[MAX_PROCESSES] = {0};
    int i;

    copy_process_set(original, &set);
    initialize_runtime_fields(&set);
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
        print_time_slice_state(&set, time, running);

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
    print_summary(&set);
}

void simulate_spn(ProcessSet *original) {
    ProcessSet set;
    int time = 0;
    int running = -1;

    copy_process_set(original, &set);
    initialize_runtime_fields(&set);
    print_header("SPN");

    while (!all_finished(&set)) {
        if (running == -1) {
            running = choose_spn(&set, time);
        }

        update_ready_states(&set, time, running);
        print_time_slice_state(&set, time, running);

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
    print_summary(&set);
}

void simulate_srt(ProcessSet *original) {
    ProcessSet set;
    int time = 0;
    int running = -1;

    copy_process_set(original, &set);
    initialize_runtime_fields(&set);
    print_header("SRT");

    while (!all_finished(&set)) {
        running = choose_srt(&set, time);
        update_ready_states(&set, time, running);
        print_time_slice_state(&set, time, running);

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
    print_summary(&set);
}

void simulate_hrrn(ProcessSet *original) {
    ProcessSet set;
    int time = 0;
    int running = -1;

    copy_process_set(original, &set);
    initialize_runtime_fields(&set);
    print_header("HRRN");

    while (!all_finished(&set)) {
        if (running == -1) {
            running = choose_hrrn(&set, time);
        }

        update_ready_states(&set, time, running);
        print_time_slice_state(&set, time, running);

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
    print_summary(&set);
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

    copy_process_set(original, &set);
    initialize_runtime_fields(&set);
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
        print_time_slice_state(&set, time, running);

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
    print_summary(&set);
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

    printf("===== Process Scheduling Demo =====\n");
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
