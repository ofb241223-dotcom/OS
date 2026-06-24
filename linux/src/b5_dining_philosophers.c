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

void philosopher_process(int semid, int id) {
    int left = id;
    int right = (id + 1) % PHILOSOPHER_COUNT;
    int first_fork;
    int second_fork;
    int round;

    /*
     * 偶数号哲学家先拿左叉子，奇数号哲学家先拿右叉子。
     * 这样可以打破“循环等待”，避免五个人同时按同一方向拿叉子造成死锁。
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
        printf("Philosopher %d is thinking. (round %d)\n", id, round);
        fflush(stdout);
        sleep(rand() % 2 + 1);

        printf("Philosopher %d is hungry.\n", id);
        fflush(stdout);

        sem_wait_op(semid, first_fork);
        printf("Philosopher %d picked up fork %d.\n", id, first_fork);
        fflush(stdout);

        sem_wait_op(semid, second_fork);
        printf("Philosopher %d picked up fork %d.\n", id, second_fork);
        printf("Philosopher %d is eating.\n", id);
        fflush(stdout);
        sleep(rand() % 2 + 1);

        sem_signal_op(semid, second_fork);
        sem_signal_op(semid, first_fork);
        printf("Philosopher %d put down forks %d and %d.\n", id, first_fork, second_fork);
        fflush(stdout);
    }

    printf("Philosopher %d finished all rounds and leaves the table.\n", id);
    fflush(stdout);
    exit(0);
}

int main(void) {
    int semid;
    union semun sem_arg;
    unsigned short values[PHILOSOPHER_COUNT];
    int i;

    for (i = 0; i < PHILOSOPHER_COUNT; ++i) {
        values[i] = 1;
    }

    semid = semget(SEM_KEY, PHILOSOPHER_COUNT, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    sem_arg.array = values;
    if (semctl(semid, 0, SETALL, sem_arg) == -1) {
        perror("semctl SETALL");
        return 1;
    }

    printf("Dining philosophers demo started.\n");
    printf("There are %d philosophers and %d fork semaphores.\n",
           PHILOSOPHER_COUNT,
           PHILOSOPHER_COUNT);
    printf("Each philosopher will eat %d times.\n\n", EAT_TIMES);
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

    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
        return 1;
    }

    printf("\nAll philosophers finished. Semaphore set removed.\n");
    return 0;
}
