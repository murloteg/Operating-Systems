#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
};

enum utils {
    CONST = 100,
    SECONDS_OF_SLEEP = 20
};

int global_initialized_int_var = CONST;
char global_initialized_char_var = 'A';

int global_uninitialized_int_var;
char global_uninitialized_char_var;

const int global_const_int_var = CONST;
const char global_const_char_var = 'B';

enum statuses execute_program() {
    pid_t process_pid = getpid();
    fprintf(stdout, "Process pid: %d\n", process_pid);

    int int_var;
    char char_var;
    fprintf(stdout, "Local var addresses: %p <---> %p\n", &int_var, &char_var);

    static int static_int_var;
    static char static_char_var;
    fprintf(stdout, "Static var addresses: %p <---> %p\n", &static_int_var, &static_char_var);

    const int const_int_val = CONST;
    const char const_char_var = 'K';
    fprintf(stdout, "Const var addresses: %p <---> %p\n", &const_int_val, &const_char_var);

    fprintf(stdout, "Global initialized var addresses: %p <---> %p\n", &global_initialized_int_var, &global_initialized_char_var);
    fprintf(stdout, "Global uninitialized var addresses: %p <---> %p\n", &global_uninitialized_int_var, &global_uninitialized_char_var);
    fprintf(stdout, "Global const var addresses: %p <---> %p\n", &global_const_int_var, &global_const_char_var);
    unsigned int status = sleep(SECONDS_OF_SLEEP);
    if (status != OK) {
        fprintf(stderr, "Woke up earlier!\n");
        return SOMETHING_WENT_WRONG;
    }
    return OK;
}

int main(int argc, char** argv) {
    enum statuses execution_status = execute_program();
    if (execution_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
