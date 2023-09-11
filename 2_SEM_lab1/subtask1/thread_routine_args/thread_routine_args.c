#include <stdio.h>
#include <stdlib.h>

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
    // TODO
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
    // TODO
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
