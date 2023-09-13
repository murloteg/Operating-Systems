#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

enum util_consts {
    INTEGER_VALUE = 100,
    TIME_TO_SLEEP_SEC = 5
};

typedef enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
} status_t;

typedef struct sample {
    int integer_value;
    char* string;
} sample;

void* thread_routine(void* arg) {
    sample* instance = (sample*) arg;
    fprintf(stdout, "[instance->integer_value: \"%d\", instance->string: \"%s\"]\n", instance->integer_value, instance->string);
    return NULL;
}

status_t execute_program() {
    sample stack_instance = {INTEGER_VALUE, "simple example!"};
    sample* instance = &stack_instance;
    if (instance == NULL) {
        return SOMETHING_WENT_WRONG;
    }
    pthread_t thread;
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

    int creation_status = pthread_create(&thread, &attributes, thread_routine, (void*) instance);
    if (creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", creation_status);
        return SOMETHING_WENT_WRONG;
    }
    int destroy_status = pthread_attr_destroy(&attributes);
    if (destroy_status != OK) {
        fprintf(stderr, "Error during pthread_attr_destroy(); error code: %d\n", destroy_status);
        return SOMETHING_WENT_WRONG;
    }

    unsigned int sleep_status = sleep(TIME_TO_SLEEP_SEC);
    if (sleep_status != OK) {
        fprintf(stderr, "Early woke up from sleep()\n");
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
