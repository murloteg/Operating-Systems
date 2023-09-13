#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

enum util_consts {
    INTEGER_VALUE = 100
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

sample* create_and_initialize_struct_instance() {
    sample* instance = malloc(sizeof(sample));
    if (instance == NULL) {
        perror("Error during malloc()");
        return NULL;
    }
    instance->integer_value = INTEGER_VALUE;
    instance->string = "simple example!";
    return instance;
}

void clean_up(sample* instance) {
    free(instance);
}

status_t execute_program() {
    sample* instance = create_and_initialize_struct_instance();
    if (instance == NULL) {
        return SOMETHING_WENT_WRONG;
    }
    pthread_t thread;
    int creation_status = pthread_create(&thread, NULL, thread_routine, (void*) instance);
    if (creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", creation_status);
        clean_up(instance);
        return SOMETHING_WENT_WRONG;
    }
    void* returned_value;
    int join_status = pthread_join(thread, &returned_value);
    if (join_status != OK) {
        fprintf(stderr, "Error during pthread_join(); error code: %d\n", join_status);
        clean_up(instance);
        return SOMETHING_WENT_WRONG;
    }
    clean_up(instance);
    return OK;
}

int main(int argc, char** argv) {
    status_t execution_status = execute_program();
    if (execution_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
