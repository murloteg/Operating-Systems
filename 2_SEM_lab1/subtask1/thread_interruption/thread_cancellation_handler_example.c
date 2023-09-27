#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

enum util_consts {
    STRING_LENGTH = 12,
    TIME_TO_SLEEP_SEC = 2,
    CALL_CLEANUP_ROUTINE = 1
};

typedef enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
} status_t;

void cleanup_routine(void* arg) {
    char* example_string = (char*) arg;
    free(example_string);
    fprintf(stdout, "Successfully clean up!\n");
}

void* thread_routine(void* arg) {
    char* example_string = (char*) calloc(sizeof(char), STRING_LENGTH);
    if (example_string == NULL) {
        perror("Error during calloc()");
        return NULL;
    }
    example_string = strncpy(example_string, "hello world", STRING_LENGTH);
    pthread_cleanup_push(cleanup_routine, example_string);
    while (true) {
        fprintf(stdout, "%s\n", example_string);
    }
    pthread_cleanup_pop(CALL_CLEANUP_ROUTINE);
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
        fprintf(stderr, "Early woke up from the sleep()\n");
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
