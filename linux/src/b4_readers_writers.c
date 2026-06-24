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

void *reader(void *arg) {
    int id = *(int *)arg;
    int round;

    for (round = 1; round <= READER_ROUNDS; ++round) {
        printf("Reader %d wants to read (round %d)\n", id, round);

        sem_wait(&mutex);
        read_count++;
        if (read_count == 1) {
            /* 第一个读者进入时阻止写者写。 */
            sem_wait(&rw_mutex);
        }
        sem_post(&mutex);

        printf("Reader %d is reading shared_data = %d\n", id, shared_data);
        usleep(150000);

        sem_wait(&mutex);
        read_count--;
        if (read_count == 0) {
            /* 最后一个读者离开时允许写者写。 */
            sem_post(&rw_mutex);
        }
        sem_post(&mutex);

        printf("Reader %d finished reading\n", id);
        usleep(100000);
    }

    return NULL;
}

void *writer(void *arg) {
    int id = *(int *)arg;
    int round;

    for (round = 1; round <= WRITER_ROUNDS; ++round) {
        printf("Writer %d wants to write (round %d)\n", id, round);

        sem_wait(&rw_mutex);
        shared_data++;
        printf("Writer %d is writing, shared_data becomes %d\n", id, shared_data);
        usleep(200000);
        sem_post(&rw_mutex);

        printf("Writer %d finished writing\n", id);
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

    if (sem_init(&mutex, 0, 1) != 0 || sem_init(&rw_mutex, 0, 1) != 0) {
        perror("sem_init");
        return 1;
    }

    printf("===== Readers-Writers Problem Demo (Reader Priority) =====\n");

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

    sem_destroy(&mutex);
    sem_destroy(&rw_mutex);

    printf("Program finished. Final shared_data = %d\n", shared_data);
    return 0;
}
