#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define PHILOSOPHER_COUNT 5
#define EAT_TIMES 3
#define SEM_KEY 0x2468

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void sem_wait_op(int semid, int semnum) {
    struct sembuf op;

    op.sem_num = semnum;
    op.sem_op = -1;
    op.sem_flg = 0;

    if (semop(semid, &op, 1) == -1) {
        perror("semop wait");
        exit(1);
    }
}

void sem_signal_op(int semid, int semnum) {
    struct sembuf op;

    op.sem_num = semnum;
    op.sem_op = 1;
    op.sem_flg = 0;

    if (semop(semid, &op, 1) == -1) {
        perror("semop signal");
        exit(1);
    }
}

void log_action(int semid, int id, int round, const char *action, const char *details) {
    int print_mutex_sem = PHILOSOPHER_COUNT; // The 6th semaphore is for print serialization
    sem_wait_op(semid, print_mutex_sem);
    printf("|   P%d   |   %d   | %-14s | %-18s |\n", id, round, action, details);
    fflush(stdout);
    sem_signal_op(semid, print_mutex_sem);
}

void philosopher_process(int semid, int id) {
    int left = id;
    int right = (id + 1) % PHILOSOPHER_COUNT;
    int first_fork;
    int second_fork;
    int round;

    /*
     * Even-numbered philosophers pick up the left fork first, odd-numbered pick up the right fork first.
     * This breaks circular wait, preventing deadlocks.
     */
    if (id % 2 == 0) {
        first_fork = left;
        second_fork = right;
    } else {
        first_fork = right;
        second_fork = left;
    }

    srand((unsigned int)(time(NULL) ^ (getpid() << 8)));

    for (round = 1; round <= EAT_TIMES; ++round) {
        // Round 1 details: log Hungry
        if (round == 1) {
            log_action(semid, id, round, "Hungry", "Wants to eat");
        }

        // Silent thinking sleep
        sleep(rand() % 2 + 1);

        sem_wait_op(semid, first_fork);
        sem_wait_op(semid, second_fork);
        
        // Eating is logged for all rounds to track progress
        log_action(semid, id, round, "Eating", "Meal in progress");
        sleep(rand() % 2 + 1);

        sem_signal_op(semid, second_fork);
        sem_signal_op(semid, first_fork);
        
        // Round 1 details: log Put Down
        if (round == 1) {
            char details[32];
            sprintf(details, "Forks %d & %d free", first_fork, second_fork);
            log_action(semid, id, round, "Put Down", details);
        }
    }

    // Always log Leaving
    log_action(semid, id, EAT_TIMES, "Leaving", "All rounds done");
    exit(0);
}

int main(void) {
    int semid;
    union semun sem_arg;
    unsigned short values[PHILOSOPHER_COUNT + 1]; // 5 forks + 1 print mutex
    int i;

    for (i = 0; i < PHILOSOPHER_COUNT; ++i) {
        values[i] = 1;
    }
    values[PHILOSOPHER_COUNT] = 1; // Print mutex semaphore set to 1

    semid = semget(SEM_KEY, PHILOSOPHER_COUNT + 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    sem_arg.array = values;
    if (semctl(semid, 0, SETALL, sem_arg) == -1) {
        perror("semctl SETALL");
        return 1;
    }

    printf("======================================================================\n");
    printf("                 DINING PHILOSOPHERS PROBLEM SIMULATION\n");
    printf("======================================================================\n");
    printf("[System Config] Philosophers: %d | Eat Times: %d | Deadlock Prevention: YES\n\n",
           PHILOSOPHER_COUNT, EAT_TIMES);
    fflush(stdout);

    printf("+--------+-------+----------------+--------------------+\n");
    printf("|  PID   | Round | Action         | Details            |\n");
    printf("+--------+-------+----------------+--------------------+\n");
    fflush(stdout);

    for (i = 0; i < PHILOSOPHER_COUNT; ++i) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            semctl(semid, 0, IPC_RMID);
            return 1;
        }

        if (pid == 0) {
            philosopher_process(semid, i);
        }
    }

    for (i = 0; i < PHILOSOPHER_COUNT; ++i) {
        wait(NULL);
    }

    printf("+--------+-------+----------------+--------------------+\n");
    printf("All philosophers finished. Semaphore set removed.\n\n");
    fflush(stdout);
    printf("\n");
    fflush(stdout);

    printf("\n==================================================\n");
    printf("                SUMMARY STATISTICS\n");
    printf("==================================================\n");
    for (i = 0; i < PHILOSOPHER_COUNT; ++i) {
        printf("Philosopher P%d ate %d times.\n", i, EAT_TIMES);
    }
    printf("==================================================\n");

    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
        return 1;
    }

    return 0;
}
