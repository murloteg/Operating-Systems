#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "list.h"

long increase_length_counter = 0;
long decrease_length_counter = 0;
long equal_length_counter = 0;
long successful_swap_counter = 0;

pthread_mutex_t sync_primitive_for_swap_counter;

void randomize_string(char *dest, size_t length) {
    char charset[] = "0123456789"
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    while (length-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *dest++ = charset[index];
    }
    *dest = '\0';
}

int get_random_length_of_string() {
    int value = (rand() % (MAX_LENGTH_OF_STRING + 1));
    return (value == 0) ? 1 : value;
}

/**
 * Natural order for string lengths
 * @param storage
 * @param values_count
 * @return
 */
status_t put_values_with_different_length_in_storage(storage_t* storage, int values_count) {
    int current_length = 1;
    for (int i = 0; i < values_count; ++i) {
        char* current_string = calloc(current_length, sizeof(char));
        if (current_string == NULL) {
            fprintf(stderr, "Error during memory allocation for string\n");
            return SOMETHING_WENT_WRONG;
        }
        randomize_string(current_string, current_length);
        storage_add(storage, current_string);
        ++current_length;
    }
    return OK;
}

/**
 * Random order for string lengths
 * @param storage
 * @param values_count
 * @return
 */
status_t put_values_with_random_length_in_storage(storage_t* storage, int values_count) {
    int current_length = get_random_length_of_string();
    for (int i = 0; i < values_count; ++i) {
        char* current_string = calloc(current_length, sizeof(char));
        if (current_string == NULL) {
            fprintf(stderr, "Error during memory allocation for string\n");
            return SOMETHING_WENT_WRONG;
        }
        randomize_string(current_string, current_length);
        storage_add(storage, current_string);
        current_length = get_random_length_of_string();
    }
    return OK;
}

status_t put_values_with_equal_length_in_storage(storage_t* storage, int values_count) {
    int total_length = 10;
    for (int i = 0; i < values_count; ++i) {
        char* current_string = calloc(total_length, sizeof(char));
        if (current_string == NULL) {
            fprintf(stderr, "Error during memory allocation for string\n");
            return SOMETHING_WENT_WRONG;
        }
        randomize_string(current_string, total_length);
        storage_add(storage, current_string);
    }
    return OK;
}

long get_swap_counter() {
    int swap_lock = pthread_mutex_lock(&sync_primitive_for_swap_counter);
    if (swap_lock != OK) {
        fprintf(stderr, "Cannot lock sync primitive!\n");
        abort();
    }
    long result = successful_swap_counter;
    int swap_unlock = pthread_mutex_unlock(&sync_primitive_for_swap_counter);
    if (swap_unlock != OK) {
        fprintf(stderr, "Cannot unlock sync primitive!\n");
        abort();
    }
    return result;
}

void* spectator_thread_routine(void* arg) {
    while (true) {
        sleep(1);
        long current_swap_operations = get_swap_counter();
        fprintf(stdout, "[STATISTIC]:\n"
                        "increase iterations ----> %ld\n"
                        "decrease_iterations ----> %ld\n"
                        "equal_iterations ----> %ld\n"
                        "TOTAL swap operations: %ld\n\n",
                increase_length_counter, decrease_length_counter, equal_length_counter, current_swap_operations);
    }
    return NULL;
}

void increase_swap_counter() {
    int swap_lock = pthread_mutex_lock(&sync_primitive_for_swap_counter);
    if (swap_lock != OK) {
        fprintf(stderr, "Cannot lock sync primitive!\n");
        return;
    }
    ++successful_swap_counter;
    int swap_unlock = pthread_mutex_unlock(&sync_primitive_for_swap_counter);
    if (swap_unlock != OK) {
        fprintf(stderr, "Cannot unlock sync primitive!\n");
        return;
    }
}

void swap_storage_nodes(storage_node_t* left_node, storage_node_t* right_node, storage_t* storage) {
    storage_node_t* previous_node = left_node->previous;
    storage_node_t* next_node = right_node->next;
    if (previous_node == NULL && next_node != NULL) {
        right_node->previous = previous_node;
        right_node->next = left_node;
        left_node->previous = right_node;
        left_node->next = next_node;
        next_node->previous = left_node;
        storage->first = right_node;
        return;
    }
    if (previous_node != NULL && next_node == NULL) {
        previous_node->next = right_node;
        right_node->previous = previous_node;
        right_node->next = left_node;
        left_node->previous = right_node;
        left_node->next = next_node;
        storage->last = left_node;
        return;
    }

    int previous_node_lock = pthread_mutex_lock(&previous_node->sync_primitive);
    int next_node_lock = pthread_mutex_lock(&next_node->sync_primitive);
    if (previous_node_lock != OK || next_node_lock != OK) {
        fprintf(stderr, "Cannot lock sync primitive!\n");
        return;
    }
    previous_node->next = right_node;
    right_node->previous = previous_node;
    right_node->next = left_node;
    next_node->previous = left_node;
    left_node->next = next_node;
    left_node->previous = right_node;

    int next_node_unlock = pthread_mutex_unlock(&next_node->sync_primitive);
    int previous_node_unlock = pthread_mutex_unlock(&previous_node->sync_primitive);
    if (previous_node_unlock != OK || next_node_unlock != OK) {
        fprintf(stderr, "Cannot unlock sync primitive!\n");
        return;
    }
}

void* increase_length_finder(void* arg) {
    storage_t* storage = (storage_t*) arg;
    int count = storage->count;
    while (true) {
//        printf("Number of swaps: %ld\n", successful_swap_counter);
        for (int i = 0; i < (count - 1); ++i) {
            storage_node_t* left_node = peek_in_storage_by_index(storage, i);
            storage_node_t* right_node = peek_in_storage_by_index(storage, i + 1);
            int left_node_lock, right_node_lock;
            if (left_node < right_node) {
                left_node_lock = pthread_mutex_lock(&left_node->sync_primitive);
                right_node_lock = pthread_mutex_lock(&right_node->sync_primitive);
            } else {
                right_node_lock = pthread_mutex_lock(&right_node->sync_primitive);
                left_node_lock = pthread_mutex_lock(&left_node->sync_primitive);
            }
//            int left_node_lock = pthread_mutex_lock(&left_node->sync_primitive);
//            int right_node_lock = pthread_mutex_lock(&right_node->sync_primitive);

            if (left_node_lock != OK || right_node_lock != OK) {
                fprintf(stderr, "Cannot lock sync primitive!\n");
                abort();
            }
            if (strlen(left_node->value) > strlen(right_node->value)) {
                swap_storage_nodes(left_node, right_node, storage);
                increase_swap_counter();
            }

            int right_node_unlock, left_node_unlock;
            if (left_node < right_node) {
                right_node_unlock = pthread_mutex_unlock(&right_node->sync_primitive);
                left_node_unlock = pthread_mutex_unlock(&left_node->sync_primitive);
            } else {
                left_node_unlock = pthread_mutex_unlock(&left_node->sync_primitive);
                right_node_unlock = pthread_mutex_unlock(&right_node->sync_primitive);
            }

//            if (left_node < right_node) {
//                left_node_unlock = pthread_mutex_unlock(&left_node->sync_primitive);
//                right_node_unlock = pthread_mutex_unlock(&right_node->sync_primitive);
//            } else {
//                right_node_unlock = pthread_mutex_unlock(&right_node->sync_primitive);
//                left_node_unlock = pthread_mutex_unlock(&left_node->sync_primitive);
//            }

//            int right_node_unlock = pthread_mutex_unlock(&right_node->sync_primitive);
//            int left_node_unlock = pthread_mutex_unlock(&left_node->sync_primitive);

            if (left_node_unlock != OK || right_node_unlock != OK) {
                fprintf(stderr, "Cannot unlock sync primitive!\n");
                abort();
            }
        }
        ++increase_length_counter;
        /* [DEBUG] */
//        print_storage(storage);
    }
    return NULL;
}

void* decrease_length_finder(void* arg) {
    storage_t* storage = (storage_t*) arg;
    int count = storage->count;
    while (true) {
//        printf("Number of swaps: %ld\n", successful_swap_counter);
        for (int i = 0; i < (count - 1); ++i) {
            storage_node_t* left_node = peek_in_storage_by_index(storage, i);
            storage_node_t* right_node = peek_in_storage_by_index(storage, i + 1);

            int left_node_lock, right_node_lock;
            if (left_node < right_node) {
                left_node_lock = pthread_mutex_lock(&left_node->sync_primitive);
                right_node_lock = pthread_mutex_lock(&right_node->sync_primitive);
            } else {
                right_node_lock = pthread_mutex_lock(&right_node->sync_primitive);
                left_node_lock = pthread_mutex_lock(&left_node->sync_primitive);
            }
//            int left_node_lock = pthread_mutex_lock(&left_node->sync_primitive);
//            int right_node_lock = pthread_mutex_lock(&right_node->sync_primitive);
            if (left_node_lock != OK || right_node_lock != OK) {
                fprintf(stderr, "Cannot lock sync primitive!\n");
                abort();
            }
            if (strlen(left_node->value) < strlen(right_node->value)) {
                swap_storage_nodes(left_node, right_node, storage);
                increase_swap_counter();
            }

            int right_node_unlock, left_node_unlock;
            if (left_node < right_node) {
                right_node_unlock = pthread_mutex_unlock(&right_node->sync_primitive);
                left_node_unlock = pthread_mutex_unlock(&left_node->sync_primitive);
            } else {
                left_node_unlock = pthread_mutex_unlock(&left_node->sync_primitive);
                right_node_unlock = pthread_mutex_unlock(&right_node->sync_primitive);
            }

//            if (left_node < right_node) {
//                left_node_unlock = pthread_mutex_unlock(&left_node->sync_primitive);
//                right_node_unlock = pthread_mutex_unlock(&right_node->sync_primitive);
//            } else {
//                right_node_unlock = pthread_mutex_unlock(&right_node->sync_primitive);
//                left_node_unlock = pthread_mutex_unlock(&left_node->sync_primitive);
//            }

//            int right_node_unlock = pthread_mutex_unlock(&right_node->sync_primitive);
//            int left_node_unlock = pthread_mutex_unlock(&left_node->sync_primitive);

            if (left_node_unlock != OK || right_node_unlock != OK) {
                fprintf(stderr, "Cannot unlock sync primitive!\n");
                abort();
            }
        }
        ++decrease_length_counter;
        /* [DEBUG] */
//        print_storage(storage);
    }
    return NULL;
}

status_t execute_program() {
    storage_t* storage = storage_init(STORAGE_SIZE);
    int init_status = pthread_mutex_init(&sync_primitive_for_swap_counter, NULL);
    if (init_status != OK) {
        fprintf(stderr, "Error during initialize sync primitive\n");
        return SOMETHING_WENT_WRONG;
    }

//    status_t put_values_status = put_values_with_different_length_in_storage(storage, 10);
//    if (put_values_status != OK) {
//        return SOMETHING_WENT_WRONG;
//    }

    status_t put_values_status = put_values_with_random_length_in_storage(storage, 10);
    if (put_values_status != OK) {
        return SOMETHING_WENT_WRONG;
    }
    print_storage(storage);

    pthread_t first_thread;
    int first_creation_status = pthread_create(&first_thread, NULL, increase_length_finder, storage);
    if (first_creation_status != OK) {
        // TODO
    }

    pthread_t second_thread;
    int second_creation_status = pthread_create(&second_thread, NULL, decrease_length_finder, storage);
    if (second_creation_status != OK) {
        // TODO
    }

    pthread_t spectator_thread;
    int spectator_creation_status = pthread_create(&spectator_thread, NULL, spectator_thread_routine, NULL);
    if (spectator_creation_status != OK) {
        // TODO
    }

    pthread_join(first_thread, NULL); // TODO
    pthread_join(second_thread, NULL); // TODO
    pthread_join(spectator_thread, NULL); // TODO

    storage_destroy(storage);
    return OK;
}

int main() {
    status_t execution_status = execute_program();
    if (execution_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
