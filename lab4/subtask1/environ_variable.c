#include <stdio.h>
#include <stdlib.h>

enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
};

enum modes {
    OVERWRITING = 1
};

enum utils {
    NUMBER_OF_REQUIRED_ARGS = 2
};

enum statuses print_environ_variable(char* environ_variable) {
    char* value = getenv(environ_variable);
    if (value == NULL) {
        fprintf(stderr, "Incorrect environ variable!\n");
        return SOMETHING_WENT_WRONG;
    }
    fprintf(stdout, "Value of environ variable: %s\n", value);
    return OK;
}

enum statuses set_environ_variable(char* environ_variable) {
    int set_status = setenv(environ_variable, "NEW VALUE", OVERWRITING);
    if (set_status == SOMETHING_WENT_WRONG) {
        perror("Error during setenv");
        return SOMETHING_WENT_WRONG;
    }
    return OK;
}

enum statuses execute_program(char* environ_variable) {
    enum statuses executing_status = print_environ_variable(environ_variable);
    if (executing_status != OK) {
        return executing_status;
    }
    executing_status = set_environ_variable(environ_variable);
    if (executing_status != OK) {
        return executing_status;
    }
    executing_status = print_environ_variable(environ_variable);
    if (executing_status != OK) {
        return executing_status;
    }
    return OK;
}

int main(int argc, char** argv) {
    if (argc != NUMBER_OF_REQUIRED_ARGS) {
        fprintf(stderr, "Incorrect number of args!\n");
        return EXIT_FAILURE;
    }

    char* environ_variable = argv[1];
    enum statuses status = execute_program(environ_variable);
    if (status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
