#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define READER_COUNT 3
#define WRITER_COUNT 2
#define READER_ROUNDS 3
#define WRITER_ROUNDS 2

int read_count = 0;
int shared_data = 0;

/* mutex 保护 read_count，rw_mutex 控制共享数据的读写。 */
sem_t mutex;
sem_t rw_mutex;
sem_t print_mutex; // Protects print statements to ensure thread safety and no line wrapping

void log_action(const char *thread_name, int round, const char *action, int active_readers, int shared_val) {
    sem_wait(&print_mutex);
    printf("| %-8s |   %d   | %-15s |       %-2d       |      %-2d     |\n",
           thread_name, round, action, active_readers, shared_val);
    sem_post(&print_mutex);
}

void *reader(void *arg) {
    int id = *(int *)arg;
    int round;
    char name[16];
    sprintf(name, "Reader %d", id);

    for (round = 1; round <= READER_ROUNDS; ++round) {
        // 1. Request access
        sem_wait(&mutex);
        int current_readers = read_count;
        sem_post(&mutex);
        log_action(name, round, "Request READ", current_readers, shared_data);

        // 2. Lock & Enter
        sem_wait(&mutex);
        read_count++;
        if (read_count == 1) {
            /* 第一个读者进入时阻止写者写。 */
            sem_wait(&rw_mutex);
        }
        current_readers = read_count;
        sem_post(&mutex);
        log_action(name, round, "Start READ", current_readers, shared_data);

        usleep(150000);

        // 3. Exit & Unlock
        sem_wait(&mutex);
        read_count--;
        if (read_count == 0) {
            /* 最后一个读者离开时允许写者写。 */
            sem_post(&rw_mutex);
        }
        current_readers = read_count;
        sem_post(&mutex);
        log_action(name, round, "End READ", current_readers, shared_data);

        usleep(100000);
    }

    return NULL;
}

void *writer(void *arg) {
    int id = *(int *)arg;
    int round;
    char name[16];
    sprintf(name, "Writer %d", id);

    for (round = 1; round <= WRITER_ROUNDS; ++round) {
        // 1. Request access (safe to acquire mutex here since we do not hold rw_mutex yet)
        sem_wait(&mutex);
        int current_readers = read_count;
        sem_post(&mutex);
        log_action(name, round, "Request WRITE", current_readers, shared_data);

        // 2. Lock & Enter (do not acquire mutex while holding rw_mutex to avoid deadlock)
        sem_wait(&rw_mutex);
        shared_data++;
        
        // Print visual block start banner
        char buf[80];
        sprintf(buf, "WRITE PHASE START: Writer %d starts writing...", id);
        sem_wait(&print_mutex);
        printf("+----------+-------+-----------------+----------------+-------------+\n");
        printf("| >>> %-61s |\n", buf);
        printf("+----------+-------+-----------------+----------------+-------------+\n");
        sem_post(&print_mutex);

        log_action(name, round, "Start WRITE", 0, shared_data);

        usleep(200000);

        // 3. Exit & Unlock
        log_action(name, round, "End WRITE", 0, shared_data);
        
        // Print visual block end banner
        sprintf(buf, "WRITE PHASE END: Writer %d finished writing.", id);
        sem_wait(&print_mutex);
        printf("+----------+-------+-----------------+----------------+-------------+\n");
        printf("| <<< %-61s |\n", buf);
        printf("+----------+-------+-----------------+----------------+-------------+\n");
        sem_post(&print_mutex);

        sem_post(&rw_mutex);

        usleep(120000);
    }

    return NULL;
}

int main(void) {
    pthread_t readers[READER_COUNT];
    pthread_t writers[WRITER_COUNT];
    int reader_ids[READER_COUNT];
    int writer_ids[WRITER_COUNT];
    int i;

    if (sem_init(&mutex, 0, 1) != 0 || sem_init(&rw_mutex, 0, 1) != 0 || sem_init(&print_mutex, 0, 1) != 0) {
        perror("sem_init");
        return 1;
    }

    printf("======================================================================\n");
    printf("                 READERS-WRITERS SIMULATION (READER PRIORITY)\n");
    printf("======================================================================\n");
    printf("[System Config] Readers: %d | Writers: %d | Reader Rounds: %d | Writer Rounds: %d\n\n",
           READER_COUNT, WRITER_COUNT, READER_ROUNDS, WRITER_ROUNDS);

    printf("+----------+-------+-----------------+----------------+-------------+\n");
    printf("| Thread   | Round | Action          | Active Readers | Shared Data |\n");
    printf("+----------+-------+-----------------+----------------+-------------+\n");

    for (i = 0; i < READER_COUNT; ++i) {
        reader_ids[i] = i + 1;
        if (pthread_create(&readers[i], NULL, reader, &reader_ids[i]) != 0) {
            perror("pthread_create reader");
            return 1;
        }
    }

    for (i = 0; i < WRITER_COUNT; ++i) {
        writer_ids[i] = i + 1;
        if (pthread_create(&writers[i], NULL, writer, &writer_ids[i]) != 0) {
            perror("pthread_create writer");
            return 1;
        }
    }

    for (i = 0; i < READER_COUNT; ++i) {
        pthread_join(readers[i], NULL);
    }

    for (i = 0; i < WRITER_COUNT; ++i) {
        pthread_join(writers[i], NULL);
    }

    printf("+----------+-------+-----------------+----------------+-------------+\n");

    sem_destroy(&mutex);
    sem_destroy(&rw_mutex);
    sem_destroy(&print_mutex);

    printf("Program finished. Final shared_data = %d\n", shared_data);
    printf("Total read operations = %d\n", READER_COUNT * READER_ROUNDS);
    printf("Total write operations = %d\n", WRITER_COUNT * WRITER_ROUNDS);
    return 0;
}
