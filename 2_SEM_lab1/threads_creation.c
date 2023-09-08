#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

enum util_consts {
    NUMBER_OF_THREADS = 5,
    GLOBAL_VARIABLE_VALUE = 1500,
    LOCAL_VARIABLE_VALUE = 700,
    NEW_VALUE_OF_VARIABLE = 4000
};

typedef enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
} status_t;

long global_var = GLOBAL_VARIABLE_VALUE;

void print_separator() {
    fprintf(stdout, "============\n\n");
}

void* thread_routine(void* arg) {
    short local_var = LOCAL_VARIABLE_VALUE;
    const short const_local_var = LOCAL_VARIABLE_VALUE;
    static short static_local_var = LOCAL_VARIABLE_VALUE;

    fprintf(stdout, "Thread routine result -> [PID: %d], [PPID: %d], [TID: %d]\n", getpid(), getppid(), pthread_self());
    fprintf(stdout, "Addresses of variables -> [GV: %p], [LV: %p], [CLV: %p], [SLV: %p]\n", &global_var,
            &local_var, &const_local_var, &static_local_var);

    fprintf(stdout, "Values BEFORE changes -> [GV: %ld], [LV: %d]\n", global_var, local_var);
    local_var = NEW_VALUE_OF_VARIABLE;
    global_var = NEW_VALUE_OF_VARIABLE;
    fprintf(stdout, "Values AFTER changes -> [GV: %ld], [LV: %d]\n", global_var, local_var);
    print_separator();
    return NULL;
}

status_t initialize_threads(pthread_t* threads) {
    for (int i = 0; i < NUMBER_OF_THREADS; ++i) {
        int creation_status = pthread_create(&threads[i], NULL, thread_routine, NULL);
        if (creation_status != OK) {
            fprintf(stderr, "Error during pthread_create(); error code: %d\n", creation_status);
            return SOMETHING_WENT_WRONG;
        }
        fprintf(stdout, "Iteration: %d, [TID: %d]\n", i, threads[i]);
    }
    return OK;
}

status_t execute_program() {
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

    unsigned int sleeping_status = sleep(NUMBER_OF_THREADS);
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
