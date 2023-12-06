#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "list.h"

long increase_length_iterations = 0;
long decrease_length_iterations = 0;
long equal_length_iterations = 0;

long increase_pairs = 0;
long decrease_pairs = 0;
long equal_pairs = 0;

long successful_swap_counter = 0;

pthread_mutex_t sync_primitive_for_swap_counter;
pthread_mutex_t storage_mutex;

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
 * Random order for string lengths
 * @param storage
 * @param values_count
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

long get_swap_counter() {
    int swap_lock = pthread_mutex_lock(&sync_primitive_for_swap_counter);
    if (swap_lock != OK) {
        fprintf(stderr, "Cannot lock sync primitive!\n");
        return INCORRECT_ACCESS_TO_SWAP_COUNTER;
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
        if (current_swap_operations == INCORRECT_ACCESS_TO_SWAP_COUNTER) {
            continue;
        }
        fprintf(stdout, "[STATISTIC]:\n"
                        "### 1 ###\n"
                        "increase iterations ----> %ld\n"
                        "decrease_iterations ----> %ld\n"
                        "equal_iterations ----> %ld\n"
                        "### 2 ###\n"
                        "increase_length_pairs ----> %ld\n"
                        "decrease_length_pairs ----> %ld\n"
                        "equal_length_pairs ----> %ld\n"
                        "### 3 ###\n"
                        "TOTAL swap operations: %ld\n\n",
                increase_length_iterations, decrease_length_iterations, equal_length_iterations,
                increase_pairs, decrease_pairs, equal_pairs, current_swap_operations);
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

void* swap_thread_routine(void* arg) {
    storage_t* storage = (storage_t *) arg;
    srand(time(NULL));
    while (true) {
        pthread_mutex_lock(&storage->first->sync_primitive);
        storage_node_t* previous_node = storage->first;
        storage_node_t* current_node = NULL;
        storage_node_t* next_node = NULL;

        while (previous_node->next != NULL){
            if ((rand() % TOTAL_NUMBER_OF_VARIANTS) == 0) {
                current_node = previous_node->next;
                pthread_mutex_lock(&current_node->sync_primitive);
                next_node = current_node->next;

                if (next_node == NULL) {
                    pthread_mutex_unlock(&current_node->sync_primitive);
                    break;
                }
                pthread_mutex_lock(&next_node->sync_primitive);
                previous_node->next = next_node;
                pthread_mutex_unlock(&previous_node->sync_primitive);
                current_node->next = next_node->next;
                pthread_mutex_unlock(&current_node->sync_primitive);
                next_node->next = current_node;

                increase_swap_counter();
                previous_node = next_node;
            } else {
                current_node = previous_node->next;
                pthread_mutex_lock(&current_node->sync_primitive);
                pthread_mutex_unlock(&previous_node->sync_primitive);
                previous_node = current_node;
            }
        }
        pthread_mutex_unlock(&previous_node->sync_primitive);
    }
}

void* increase_length_finder(void* arg) {
    storage_t* storage = (storage_t*) arg;
    while (true) {
        pthread_mutex_lock(&storage->first->sync_primitive);
        storage_node_t* previous_node = storage->first;
        storage_node_t* current = NULL;
        while (previous_node->next != NULL) {
            current = previous_node->next;
            unsigned int size = strlen(previous_node->value);
            pthread_mutex_lock(&current->sync_primitive);
            pthread_mutex_unlock(&previous_node->sync_primitive);
            if (size < strlen(current->value)) {
                ++increase_pairs;
            }
            previous_node = current;
        }
        ++increase_length_iterations;
        pthread_mutex_unlock(&previous_node->sync_primitive);
    }
    return NULL;
}

void* decrease_length_finder(void* arg) {
    storage_t* storage = (storage_t*) arg;
    while (true) {
        pthread_mutex_lock(&storage->first->sync_primitive);
        storage_node_t* previous_node = storage->first;
        storage_node_t* current;
        while (previous_node->next != NULL) {
            current = previous_node->next;
            unsigned int size = strlen(previous_node->value);
            pthread_mutex_lock(&current->sync_primitive);
            pthread_mutex_unlock(&previous_node->sync_primitive);
            if (size > strlen(current->value)) {
                ++decrease_pairs;
            }
            previous_node = current;
        }
        ++decrease_length_iterations;
        pthread_mutex_unlock(&previous_node->sync_primitive);
    }
    return NULL;
}

void* equal_length_finder(void* arg) {
    storage_t* storage = (storage_t*) arg;
    while (true) {
        pthread_mutex_lock(&storage->first->sync_primitive);
        storage_node_t* previous_node = storage->first;
        storage_node_t* current;
        while (previous_node->next != NULL) {
            current = previous_node->next;
            unsigned int size = strlen(previous_node->value);
            pthread_mutex_lock(&current->sync_primitive);
            pthread_mutex_unlock(&previous_node->sync_primitive);
            if (size == strlen(current->value)) {
                ++equal_pairs;
            }
            previous_node = current;
        }
        ++equal_length_iterations;
        pthread_mutex_unlock(&previous_node->sync_primitive);
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

    init_status = pthread_mutex_init(&storage_mutex, NULL);
    if (init_status != OK) {
        fprintf(stderr, "Error during initialize sync primitive\n");
        return SOMETHING_WENT_WRONG;
    }

    status_t put_values_status = put_values_with_random_length_in_storage(storage, STORAGE_SIZE);
    if (put_values_status != OK) {
        return SOMETHING_WENT_WRONG;
    }
    print_storage(storage);

    pthread_t first_thread;
    int first_creation_status = pthread_create(&first_thread, NULL, increase_length_finder, storage);
    if (first_creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", first_creation_status);
        return SOMETHING_WENT_WRONG;
    }

    pthread_t second_thread;
    int second_creation_status = pthread_create(&second_thread, NULL, decrease_length_finder, storage);
    if (second_creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", second_creation_status);
        return SOMETHING_WENT_WRONG;
    }

    pthread_t third_thread;
    int third_creation_status = pthread_create(&third_thread, NULL, equal_length_finder, storage);
    if (third_creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", third_creation_status);
        return SOMETHING_WENT_WRONG;
    }

    pthread_t spectator_thread;
    int spectator_creation_status = pthread_create(&spectator_thread, NULL, spectator_thread_routine, NULL);
    if (spectator_creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", spectator_creation_status);
        return SOMETHING_WENT_WRONG;
    }

    pthread_t first_reshuffler;
    int reshuffler_creation_status = pthread_create(&first_reshuffler, NULL, swap_thread_routine, storage);
    if (reshuffler_creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", reshuffler_creation_status);
        return SOMETHING_WENT_WRONG;
    }

    pthread_t second_reshuffler;
    reshuffler_creation_status = pthread_create(&second_reshuffler, NULL, swap_thread_routine, storage);
    if (reshuffler_creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", reshuffler_creation_status);
        return SOMETHING_WENT_WRONG;
    }

    pthread_t third_reshuffler;
    reshuffler_creation_status = pthread_create(&third_reshuffler, NULL, swap_thread_routine, storage);
    if (reshuffler_creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", reshuffler_creation_status);
        return SOMETHING_WENT_WRONG;
    }

    sleep(5);
    int storage_lock_status = pthread_mutex_lock(&storage_mutex);
    if (storage_lock_status != OK) {
        fprintf(stderr, "Error during pthread_mutex_lock(); error code: %d\n", storage_lock_status);
        return SOMETHING_WENT_WRONG;
    }
    print_storage(storage);
    int storage_unlock_status = pthread_mutex_unlock(&storage_mutex);
    if (storage_unlock_status != OK) {
        fprintf(stderr, "Error during pthread_mutex_unlock(); error code: %d\n", storage_unlock_status);
        return SOMETHING_WENT_WRONG;
    }

    int first_join_status = pthread_join(first_thread, NULL);
    if (first_join_status != OK) {
        fprintf(stderr, "Error during pthread_join()\n");
        return SOMETHING_WENT_WRONG;
    }

    int second_join_status = pthread_join(second_thread, NULL);
    if (second_join_status != OK) {
        fprintf(stderr, "Error during pthread_join()\n");
        return SOMETHING_WENT_WRONG;
    }

    int third_join_status = pthread_join(third_thread, NULL);
    if (third_join_status != OK) {
        fprintf(stderr, "Error during pthread_join()\n");
        return SOMETHING_WENT_WRONG;
    }

    int spectator_join_status = pthread_join(spectator_thread, NULL);
    if (spectator_join_status != OK) {
        fprintf(stderr, "Error during pthread_join()\n");
        return SOMETHING_WENT_WRONG;
    }

    int reshuffler_join_status = pthread_join(first_reshuffler, NULL);
    if (reshuffler_join_status != OK) {
        fprintf(stderr, "Error during pthread_join()\n");
        return SOMETHING_WENT_WRONG;
    }

    reshuffler_join_status = pthread_join(second_reshuffler, NULL);
    if (reshuffler_join_status != OK) {
        fprintf(stderr, "Error during pthread_join()\n");
        return SOMETHING_WENT_WRONG;
    }

    reshuffler_join_status = pthread_join(third_reshuffler, NULL);
    if (reshuffler_join_status != OK) {
        fprintf(stderr, "Error during pthread_join()\n");
        return SOMETHING_WENT_WRONG;
    }

    int destroy_status = pthread_mutex_destroy(&storage_mutex);
    if (destroy_status != OK) {
        fprintf(stderr, "Error during pthread_mutex_destroy(); error code: %d\n", destroy_status);
        return SOMETHING_WENT_WRONG;
    }

    destroy_status = pthread_mutex_destroy(&sync_primitive_for_swap_counter);
    if (destroy_status != OK) {
        fprintf(stderr, "Error during pthread_mutex_destroy(); error code: %d\n", destroy_status);
        return SOMETHING_WENT_WRONG;
    }

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
