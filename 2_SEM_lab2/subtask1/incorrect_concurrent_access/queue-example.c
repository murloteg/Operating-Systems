#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>

#include "queue.h"

status_t execute_program() {
    fprintf(stdout, "MAIN-THREAD: [%d %d %d]\n\n", getpid(), getppid(), gettid());
    queue_t *queue = queue_init(QUEUE_SIZE);

    for (int i = 0; i < 10; ++i) {
        status_t adding_status = queue_add(queue, i);
        if (adding_status == OK) {
            fprintf(stdout, "Value added successfully! Value: %d\n", i);
        } else {
            fprintf(stdout, "Value wasn't added! Value: %d\n", i);
        }
        queue_print_stats(queue);
    }

    for (int i = 0; i < 12; ++i) {
        int value = -1;
        status_t getting_status = queue_get(queue, &value);
        if (getting_status == OK) {
            fprintf(stdout, "Value was got successfully! Value: %d\n", value);
        } else {
            fprintf(stdout, "Value wasn't got! Value: %d\n", value);
        }
        queue_print_stats(queue);
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
