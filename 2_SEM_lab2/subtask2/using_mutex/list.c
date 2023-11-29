#include <pthread.h>

#include "list.h"

pthread_mutex_t access_sync_primitive;

storage_t* storage_init(int max_count) {
    storage_t* storage = (storage_t*) malloc(sizeof(storage_t));
    if (storage == NULL) {
        perror("Error during malloc()");
        abort();
    }

    pthread_mutex_init(&access_sync_primitive, NULL); // TODO

    storage->first = NULL;
    storage->last = NULL;
    storage->max_count = max_count;
    storage->count = 0;

    return storage;
}

int storage_add(storage_t* storage, char* value) {
    if (strlen(value) > MAX_LENGTH_OF_STRING) {
        fprintf(stderr, "Value has length > MAX_LENGTH_OF_TEXT\n");
        return SOMETHING_WENT_WRONG;
    }

    if (storage->count == storage->max_count) {
        return SOMETHING_WENT_WRONG;
    }

    storage_node_t* new_element = (storage_node_t*) malloc(sizeof(storage_node_t));
    if (new_element == NULL) {
        perror("Error during malloc()");
        abort();
    }

    new_element->value = value;
    new_element->next = NULL;
    new_element->previous = NULL;
    int sync_primitive_init_status = pthread_mutex_init(&new_element->sync_primitive, NULL);
    if (sync_primitive_init_status != OK) {
        fprintf(stderr, "Error during initialize sync primitive\n");
        return SOMETHING_WENT_WRONG;
    }

    if (!storage->first) {
        storage->first = storage->last = new_element;
    } else {
        storage->last->next = new_element;
        new_element->previous = storage->last;
        storage->last = storage->last->next;
    }
    ++storage->count;
    return OK;
}

storage_node_t* peek_in_storage_by_index(storage_t* storage, int index) {
    storage_node_t* node;
    pthread_mutex_lock(&access_sync_primitive); // TODO
    if (storage->count == 0 || index >= storage->max_count) {
        return NULL;
    }
    node = storage->first;
    for (int i = 0; i < index; ++i) {
        node = node->next;
    }
    pthread_mutex_unlock(&access_sync_primitive); // TODO
    return node;
}

void storage_destroy(storage_t* storage) {
    if (storage != NULL) {
        storage_node_t* node = storage->first;
        while (node != NULL) {
            free(node->value);
            int destroy_sync_primitive_status = pthread_mutex_destroy(&node->sync_primitive);
            if (destroy_sync_primitive_status != OK) {
                // TODO;
            }

            storage_node_t* next_node = node->next;
            free(node);
            node = next_node;
        }
    }
    free(storage);
}

void print_storage(storage_t* storage) {
    int index = 0;
    storage_node_t* current_node = storage->first;
    while (current_node != NULL) {
        fprintf(stdout, "[%d]: %s\n", index, current_node->value);
        current_node = current_node->next;
        ++index;
    }
    fprintf(stdout, "\n");
}
