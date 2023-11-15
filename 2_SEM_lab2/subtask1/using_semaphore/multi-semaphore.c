#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>

#include "queue.h"

#define RED "\033[41m"
#define NOCOLOR "\033[0m"

sem_t semaphore;

void set_cpu(int n) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(n, &cpuset);

    pthread_t tid = pthread_self();
    int setting_status = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
    if (setting_status != OK) {
        fprintf(stderr, "set_cpu: pthread_setaffinity failed for cpu %d\n", n);
        return;
    }
    fprintf(stdout, "set_cpu: set cpu %d\n", n);
}

void* reader(void* arg) {
    printf("READER-THREAD [%d %d %d]\n", getpid(), getppid(), gettid());

    int expected = 0;
    queue_t* queue = (queue_t*) arg;
    set_cpu(1);

    while (true) {
        int value = -1;
        sem_wait(&semaphore);
        status_t getting_status = queue_get(queue, &value);
        sem_post(&semaphore);
        if (getting_status != OK) {
            continue;
        }

        if (expected != value) {
            fprintf(stdout, RED"ERROR: get value is %d but expected - %d" NOCOLOR "\n", value, expected);
        }
        expected = value + 1;
    }
    return NULL;
}

void* writer(void* arg) {
    printf("WRITER-THREAD [%d %d %d]\n", getpid(), getppid(), gettid());

    queue_t* queue = (queue_t*) arg;
    set_cpu(2);

    int value = 0;
    while (true) {
        sem_wait(&semaphore);
//        usleep(1);
        status_t adding_status = queue_add(queue, value);
        sem_post(&semaphore);
        if (adding_status != OK) {
            continue;
        }
        ++value;
    }
    return NULL;
}

void* stub(void* arg) {
    printf("STUB-THREAD [%d %d %d]\n", getpid(), getppid(), gettid());
    while (true) {
        sem_wait(&semaphore);
        usleep(50);
        fprintf(stdout, "Stub thread finished iteration...\n");
        sem_post(&semaphore);
    }
    return NULL;
}

status_t execute_program() {
    printf("MAIN-THREAD [%d %d %d]\n\n", getpid(), getppid(), gettid());

    queue_t* queue = queue_init(QUEUE_SIZE);
    int semaphore_initialization_status = sem_init(&semaphore, 0, 2);
    if (semaphore_initialization_status != OK) {
        fprintf(stderr, "Error during initializing of semaphore\n");
        abort();
    }

    pthread_t reader_tid;
    status_t creation_status = pthread_create(&reader_tid, NULL, reader, queue);
    if (creation_status != OK) {
        fprintf(stderr, "main: pthread_create() for reader failed with code: %d\n", creation_status);
        return SOMETHING_WENT_WRONG;
    }

    int scheduling_status = sched_yield();
    if (scheduling_status != OK) {
        perror("Error during sched_yield()");
        return SOMETHING_WENT_WRONG;
    }

    pthread_t writer_tid;
    creation_status = pthread_create(&writer_tid, NULL, writer, queue);
    if (creation_status != OK) {
        fprintf(stderr, "main: pthread_create() for writer failed with code: %d\n", creation_status);
        return SOMETHING_WENT_WRONG;
    }

    void* reader_return_value = NULL;
    int reader_join_status = pthread_join(reader_tid, &reader_return_value);
    if (reader_join_status != OK) {
        fprintf(stderr, "Error during pthread_join() for reader thread. Error code: %d\n", reader_join_status);
        return SOMETHING_WENT_WRONG;
    }

    void* writer_return_value = NULL;
    int writer_join_status = pthread_join(writer_tid, &writer_return_value);
    if (writer_join_status != OK) {
        fprintf(stderr, "Error during pthread_join() for writer thread. Error code: %d\n", writer_join_status);
        return SOMETHING_WENT_WRONG;
    }
    queue_destroy(queue);
    int destroy_status = sem_destroy(&semaphore);
    if (destroy_status != OK) {
        fprintf(stderr, "Error during sem_destroy(); error code: %d\n", destroy_status);
    }
    return OK;
}

int main() {
    status_t executing_status = execute_program();
    if (executing_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
