#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <sched.h>
#include <signal.h>

enum util_consts {
    DELAY_TIME_IN_SEC = 1,
    NUMBER_OF_ITERATIONS = 5,
    INITIAL_THREAD_ID = 1,
    NUMBER_OF_THREADS = 3,
    BUFFER_SIZE = 512,
    STACK_SIZE = 65536
};

enum permission_modes {
    READ_WRITE_OWNER_AND_GROUP = 0660
};

typedef enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
} status_t;

typedef void* (*thread_routine_t)(void*);

typedef struct custom_thread {
    int thread_id;
    thread_routine_t thread_routine;
    void* argument;
    void* return_value;
    volatile bool is_thread_finished;
    volatile bool is_thread_joined;
} custom_thread_t;

static int thread_id = INITIAL_THREAD_ID;

int custom_thread_startup(void* argument) {
    custom_thread_t* thread = (custom_thread_t*) argument;
    void* return_value = thread->thread_routine(thread->argument);
    thread->return_value = return_value;
    thread->is_thread_finished = true;

    while (!thread->is_thread_joined) {
        sleep(DELAY_TIME_IN_SEC);
    }
    fprintf(stdout, "Successful finish of job for thread with TID: %d\n", thread->thread_id);
    return OK;
}

void* simple_thread_routine(void* argument) {
    long current_thread_id = (long) argument;
    for (int i = 0; i < NUMBER_OF_ITERATIONS; ++i) {
        fprintf(stdout, "Custom thread with TID: %ld still alive!\n", current_thread_id);
        sleep(DELAY_TIME_IN_SEC);
    }
    return (void*) ">>>SOME RESPONSE MESSAGE<<<";
}

void* create_stack(off_t size, int current_thread_id) {
    char stack_file[BUFFER_SIZE];
    snprintf(stack_file, sizeof(stack_file), "stack-%d", current_thread_id);

    int stack_file_descriptor = open(stack_file, O_RDWR | O_CREAT, READ_WRITE_OWNER_AND_GROUP);
    if (stack_file_descriptor == SOMETHING_WENT_WRONG) {
        perror("Error during open()");
        return NULL;
    }
    int truncate_status = ftruncate(stack_file_descriptor, 0);
    if (truncate_status == SOMETHING_WENT_WRONG) {
        perror("Error during ftruncate()");
        close(stack_file_descriptor);
        return NULL;
    }
    truncate_status = ftruncate(stack_file_descriptor, size);
    if (truncate_status == SOMETHING_WENT_WRONG) {
        perror("Error during ftruncate()");
        close(stack_file_descriptor);
        return NULL;
    }

    void* stack = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, stack_file_descriptor, 0);
    if (stack == MAP_FAILED) {
        perror("Error during mmap()");
        close(stack_file_descriptor);
        return NULL;
    }

    int closing_status = close(stack_file_descriptor);
    if (closing_status == SOMETHING_WENT_WRONG) {
        perror("Error during close()");
        munmap(stack, size);
        return NULL;
    }
    fprintf(stdout, "Successful creation of stack for custom thread with TID: %d!\n", current_thread_id);
    return stack;
}

int get_and_increase_thread_id() {
    int current_thread_id = thread_id;
    ++thread_id;
    return current_thread_id;
}

int get_thread_id_without_increase() {
    return thread_id;
}

int custom_thread_create(custom_thread_t* custom_thread, thread_routine_t thread_routine, void* argument) {
    int current_thread_id = get_and_increase_thread_id();
    void* stack = create_stack(STACK_SIZE, current_thread_id);
    if (stack == NULL) {
        return SOMETHING_WENT_WRONG;
    }

    custom_thread->thread_id = current_thread_id;
    custom_thread->thread_routine = simple_thread_routine;
    custom_thread->argument = argument;
    custom_thread->is_thread_finished = false;
    custom_thread->is_thread_joined = false;
    custom_thread->return_value = NULL;

    int clone_status = clone(custom_thread_startup, stack + STACK_SIZE, CLONE_VM | CLONE_FILES | CLONE_THREAD | CLONE_SIGHAND | SIGCHLD, (void*) custom_thread);
    if (clone_status == SOMETHING_WENT_WRONG) {
        perror("Error during clone()");
        munmap(stack, STACK_SIZE);
        return SOMETHING_WENT_WRONG;
    }

    /* Stack unmap section
    int unmap_status = munmap(stack, STACK_SIZE);
    if (unmap_status == SOMETHING_WENT_WRONG) {
        perror("Error during munmap()");
        return SOMETHING_WENT_WRONG;
    }
     */
    fprintf(stdout, "Successful creation of custom thread with TID: %d!\n", current_thread_id);
    return OK;
}

int custom_thread_join(custom_thread_t* custom_thread, void** return_value) {
    while (!custom_thread->is_thread_finished) {
        sleep(DELAY_TIME_IN_SEC);
    }
    *return_value = custom_thread->return_value;
    custom_thread->is_thread_finished = true;
    fprintf(stdout, "Successful join for custom thread with TID: %d!\n", custom_thread->thread_id);
    return OK;
}

void clean_up(custom_thread_t** array_of_threads) {
    for (int i = 0; i < NUMBER_OF_THREADS; ++i) {
        free(array_of_threads[i]);
    }
    free(array_of_threads);
}

status_t initialize_array(custom_thread_t** array_of_threads) {
    for (int i = 0; i < NUMBER_OF_THREADS; ++i) {
        array_of_threads[i] = (custom_thread_t*) malloc(sizeof(custom_thread_t));
        if (array_of_threads[i] == NULL) {
            perror("Error during malloc()");
            return SOMETHING_WENT_WRONG;
        }
    }
    for (int i = 0; i < NUMBER_OF_THREADS; ++i) {
        int creation_status = custom_thread_create(array_of_threads[i], simple_thread_routine, (void*) (long) get_thread_id_without_increase());
        if (creation_status != OK) {
            clean_up(array_of_threads);
            return SOMETHING_WENT_WRONG;
        }
    }
    return OK;
}

status_t execute_program() {
    custom_thread_t** array_of_threads = (custom_thread_t**) malloc(NUMBER_OF_THREADS * sizeof(custom_thread_t));
    if (array_of_threads == NULL) {
        perror("Error during malloc()");
        return SOMETHING_WENT_WRONG;
    }

    status_t initialization_status = initialize_array(array_of_threads);
    if (initialization_status != OK) {
        return SOMETHING_WENT_WRONG;
    }

    for (int i = 0; i < NUMBER_OF_THREADS; ++i) {
        void* return_value;
        int join_status = custom_thread_join(array_of_threads[i], &return_value);
        if (join_status != OK) {
            fprintf(stderr, "Error during custom_thread_join()\n");
            clean_up(array_of_threads);
            return SOMETHING_WENT_WRONG;
        }
        if (return_value != NULL) {
            fprintf(stdout, "Custom thread with TID: %d finished with return value: %s\n", array_of_threads[i]->thread_id, (char*) return_value);
        }
    }
    clean_up(array_of_threads);
    return OK;
}

int main(int argc, char** argv) {
    status_t execution_status = execute_program();
    if (execution_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
