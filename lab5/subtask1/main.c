#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
};

enum consts {
    OLD_VALUE = 150,
    NEW_VALUE = 200
};

enum utils {
    CHILD_PID_AFTER_FORK = 0,
    PARENT_DELAY_TIME_IN_SEC = 30,
    CHILD_DELAY_TIME_IN_SEC = 15,
    CHILD_EXIT_CODE = 5,
    BLOCKING_WAIT = 0
};

int global_int_var;

enum statuses execute_program() {
    global_int_var = OLD_VALUE;
    int local_int_var = OLD_VALUE;

    fprintf(stdout, "Global var: address %p ; value %d\n", &global_int_var, global_int_var);
    fprintf(stdout, "Local var: address %p ; value %d\n", &local_int_var, local_int_var);
    fprintf(stdout, "Process ID: %d\n", getpid());
    pid_t child_pid = fork();
    if (child_pid == SOMETHING_WENT_WRONG) {
        perror("Error during fork");
        return SOMETHING_WENT_WRONG;
    } else if (child_pid == CHILD_PID_AFTER_FORK) {
        fprintf(stdout, "Child's PID: %d\n", getpid());
        fprintf(stdout, "Child's parent PID: %d\n", getppid());

        fprintf(stdout, "[CHILD] global var: address %p ; old value %d\n", &global_int_var, global_int_var);
        fprintf(stdout, "[CHILD] local var: address %p ; old value %d\n", &local_int_var, local_int_var);
        local_int_var = NEW_VALUE;
        global_int_var = NEW_VALUE;
        fprintf(stdout, "[CHILD] global var: address %p ; new value %d\n", &global_int_var, global_int_var);
        fprintf(stdout, "[CHILD] local var: address %p ; new value %d\n", &local_int_var, local_int_var);
        unsigned int status = sleep(CHILD_DELAY_TIME_IN_SEC);
        if (status != OK) {
            fprintf(stderr, "Woke up earlier!\n");
            return SOMETHING_WENT_WRONG;
        }
        exit(CHILD_EXIT_CODE);
    } else {
        fprintf(stdout, "[PARENT] global var: address %p ; value %d\n", &global_int_var, global_int_var);
        fprintf(stdout, "[PARENT] local var: address %p ; value %d\n", &local_int_var, local_int_var);
        unsigned int status = sleep(PARENT_DELAY_TIME_IN_SEC);
        if (status != OK) {
            fprintf(stderr, "Woke up earlier!\n");
            return SOMETHING_WENT_WRONG;
        }

        int exit_status;
        pid_t returned_pid = waitpid(child_pid, &exit_status, BLOCKING_WAIT);
        if (returned_pid == SOMETHING_WENT_WRONG) {
            perror("Error during waitpid");
            return SOMETHING_WENT_WRONG;
        }
        if (WIFEXITED(exit_status)) {
            fprintf(stdout, "Child process terminated with exit code %d\n", WEXITSTATUS(exit_status));
        } else if (WIFSIGNALED(exit_status)) {
            fprintf(stdout, "Child process terminated by signal %d\n", WTERMSIG(exit_status));
        } else {
            fprintf(stderr, "Child process was stopped by another cause\n");
            return SOMETHING_WENT_WRONG;
        }
    }
    return OK;
}

int main(int argc, char** argv) {
    enum statuses executing_status = execute_program();
    if (executing_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
