#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

enum util_consts {
    TIME_TO_SLEEP_SEC = 2
};

typedef enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
} status_t;

void* thread_routine(void* arg) {
    char* string = "simple example!";
    while (true) {
        fprintf(stdout, "%s\n", string);
    }
    return NULL;
}

status_t execute_program() {
    pthread_t thread;
    int creation_status = pthread_create(&thread, NULL, thread_routine, NULL);
    if (creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", creation_status);
        return SOMETHING_WENT_WRONG;
    }
    unsigned int sleep_status = sleep(TIME_TO_SLEEP_SEC);
    if (sleep_status != OK) {
        fprintf(stderr, "Early woke up from sleep()\n");
        return SOMETHING_WENT_WRONG;
    }
    int cancel_status = pthread_cancel(thread);
    if (cancel_status != OK) {
        fprintf(stderr, "Error during pthread_cancel(); error code: %d\n", cancel_status);
        return SOMETHING_WENT_WRONG;
    }
    void* returned_value;
    int join_status = pthread_join(thread, &returned_value);
    if (join_status != OK) {
        fprintf(stderr, "Error during pthread_join(); error code: %d\n", join_status);
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
