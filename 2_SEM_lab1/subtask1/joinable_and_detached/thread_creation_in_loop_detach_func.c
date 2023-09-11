#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

typedef enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
} status_t;

void* thread_routine(void* arg) {
    pthread_t thread = pthread_self();
    fprintf(stdout, "[TID: %ld]\n", thread);
    /* This section for 1.2.e */
    int detach_status = pthread_detach(thread);
    if (detach_status != OK) {
        fprintf(stderr, "Error during pthread_detach(); error code: %d\n", detach_status);
    }
    return NULL;
}

status_t execute_program() {
    pthread_t thread;
    while (true) {
        /* This section for 1.2.a-e */
        int creation_status = pthread_create(&thread, NULL, thread_routine, NULL);
        if (creation_status != OK) {
            fprintf(stderr, "Error during pthread_create(); error code: %d\n", creation_status);
            return SOMETHING_WENT_WRONG;
        }
    }
    return OK;
}

int main(int argc, char** argv) {
    status_t execution_status = execute_program();
    if (execution_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
