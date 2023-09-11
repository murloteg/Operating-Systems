#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

enum util_consts {
    NUMBER_OF_THREADS = 5,
    GLOBAL_VARIABLE_VALUE = 1500,
    LOCAL_VARIABLE_VALUE = 700,
    NEW_VALUE_OF_VARIABLE = 4000,
    TIME_TO_SLEEP_SEC = 15
};

typedef enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
} status_t;

long global_var = GLOBAL_VARIABLE_VALUE;

void* thread_routine(void* arg) {
    short local_var = LOCAL_VARIABLE_VALUE;
    const short const_local_var = LOCAL_VARIABLE_VALUE;
    static short static_local_var = LOCAL_VARIABLE_VALUE;

    fprintf(stdout, "Thread routine result -> [PID: %d], [PPID: %d], [TID: %ld]\n", getpid(), getppid(), pthread_self());
    fprintf(stdout, "Addresses of variables -> [GV: %p], [LV: %p], [CLV: %p], [SLV: %p]\n", &global_var,
            &local_var, &const_local_var, &static_local_var);

    fprintf(stdout, "Values BEFORE changes -> [GV: %ld], [LV: %d]\n", global_var, local_var);
    local_var = NEW_VALUE_OF_VARIABLE;
    global_var = NEW_VALUE_OF_VARIABLE;
    fprintf(stdout, "Values AFTER changes -> [GV: %ld], [LV: %d]\n\n\n", global_var, local_var);
    return NULL;
}

status_t initialize_threads(pthread_t* threads) {
    for (int i = 0; i < NUMBER_OF_THREADS; ++i) {
        int creation_status = pthread_create(&threads[i], NULL, thread_routine, NULL);
        if (creation_status != OK) {
            fprintf(stderr, "Error during pthread_create(); error code: %d\n", creation_status);
            return SOMETHING_WENT_WRONG;
        }
        fprintf(stdout, "Iteration: %d, [TID: %ld]\n", i, threads[i]);
    }
    return OK;
}

status_t execute_program() {
    fprintf(stdout, ">>> Main Thread PID: %d <<<\n", getpid());
    unsigned int sleeping_status = sleep(TIME_TO_SLEEP_SEC);
    if (sleeping_status != OK) {
        fprintf(stderr, "Error during sleep()");
    }

    pthread_t* threads = malloc(sizeof(pthread_t) * NUMBER_OF_THREADS);
    if (threads == NULL) {
        perror("Error during malloc()");
        return SOMETHING_WENT_WRONG;
    }

    status_t initialization_status = initialize_threads(threads);
    if (initialization_status != OK) {
        free(threads);
        return SOMETHING_WENT_WRONG;
    }

    sleeping_status = sleep(TIME_TO_SLEEP_SEC);
    if (sleeping_status != OK) {
        fprintf(stderr, "Error during sleep()");
    }
    free(threads);
    return OK;
}

int main(int argc, char** argv) {
    status_t execution_status = execute_program();
    if (execution_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
