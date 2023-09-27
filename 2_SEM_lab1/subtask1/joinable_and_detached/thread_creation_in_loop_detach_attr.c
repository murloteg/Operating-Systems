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
    return NULL;
}

status_t execute_program() {
    pthread_t thread;
    /* This section for 1.2.f */
    pthread_attr_t attributes;
    int initialization_status = pthread_attr_init(&attributes);
    if (initialization_status != OK) {
        fprintf(stderr, "Error during pthread_attr_init(); error code: %d\n", initialization_status);
    }
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

    while (true) {
        /* This section for 1.2.f */
        int creation_status = pthread_create(&thread, &attributes, thread_routine, NULL);
        if (creation_status != OK) {
            fprintf(stderr, "Error during pthread_create(); error code: %d\n", creation_status);
            /* This section for 1.2.f */
            pthread_attr_destroy(&attributes);
            return SOMETHING_WENT_WRONG;
        }
    }
    /* This section for 1.2.f */
    int destroy_status = pthread_attr_destroy(&attributes);
    if (destroy_status != OK) {
        fprintf(stderr, "Error during pthread_attr_destroy(); error code: %d\n", destroy_status);
        return SOMETHING_WENT_WRONG;
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
