#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

enum util_consts {
    RETURN_CODE = 42
};

typedef enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
} status_t;

void* thread_routine(void* arg) {
    fprintf(stdout, "Hello from thread routine!\n");
    /* This section for 1.2.b */
//    return (void*) RETURN_CODE;
    char* return_string = "hello world";
    return (void*) return_string;
}

status_t execute_program() {
    pthread_attr_t attributes;
    int initialization_status = pthread_attr_init(&attributes);
    if (initialization_status != OK) {
        fprintf(stderr, "Error during pthread_attr_init(); error code: %d\n", initialization_status);
        return SOMETHING_WENT_WRONG;
    }
    int setting_status = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);
    if (setting_status != OK) {
        fprintf(stderr, "Error during pthread_attr_setdetachstate(); error code: %d\n", setting_status);
        pthread_attr_destroy(&attributes);
        return SOMETHING_WENT_WRONG;
    }

    pthread_t thread;
    int creation_status = pthread_create(&thread, &attributes, thread_routine, NULL);
    if (creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", creation_status);
        pthread_attr_destroy(&attributes);
        return SOMETHING_WENT_WRONG;
    }

    int destroy_status = pthread_attr_destroy(&attributes);
    if (destroy_status != OK) {
        fprintf(stderr, "Error during pthread_attr_destroy(); error code: %d\n", destroy_status);
        return SOMETHING_WENT_WRONG;
    }

    void* thread_exit_code;
    int joining_status = pthread_join(thread, &thread_exit_code);
    if (joining_status != OK) {
        fprintf(stderr, "Error during pthread_join(); error code: %d\n", joining_status);
        return SOMETHING_WENT_WRONG;
    }
    /* This section for 1.2.b */
//    fprintf(stdout, "Thread joined with exit code: %ld\n", (long) thread_exit_code);
    fprintf(stdout, "Thread joined with exit code: %s\n", (char*) thread_exit_code);
    return OK;
}

int main(int argc, char** argv) {
    status_t execution_status = execute_program();
    if (execution_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
