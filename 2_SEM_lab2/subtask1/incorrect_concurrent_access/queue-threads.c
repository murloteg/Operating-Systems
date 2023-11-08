#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>

#include "queue.h"

#define RED "\033[41m"
#define NOCOLOR "\033[0m"

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
		status_t getting_status = queue_get(queue, &value);
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
	set_cpu(1);

    int value = 0;
	while (true) {
		status_t adding_status = queue_add(queue, value);
		if (adding_status != OK) {
            continue;
        }
        ++value;
	}
	return NULL;
}

status_t execute_program() {
    printf("MAIN-THREAD [%d %d %d]\n\n", getpid(), getppid(), gettid());

    queue_t* queue = queue_init(QUEUE_SIZE);
    pthread_t reader_tid;
    status_t creation_status = pthread_create(&reader_tid, NULL, reader, queue);
    if (creation_status != OK) {
        fprintf(stderr, "main: pthread_create() for reader failed with code: %d\n", creation_status);
        return SOMETHING_WENT_WRONG;
    }
    struct timespec reader_start_time;
    clock_gettime (CLOCK_REALTIME, &reader_start_time);
    fprintf(stdout, "Writer TIME: [%ld s., %ld ns]\n", reader_start_time.tv_sec / 1000000000, reader_start_time.tv_nsec);

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
    struct timespec writer_start_time;
    clock_gettime (CLOCK_REALTIME, &writer_start_time);
    fprintf(stdout, "Writer TIME: [%ld s., %ld ns]\n", writer_start_time.tv_sec / 1000000000, writer_start_time.tv_nsec);

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
    return OK;
}

int main() {
    status_t executing_status = execute_program();
    if (executing_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
