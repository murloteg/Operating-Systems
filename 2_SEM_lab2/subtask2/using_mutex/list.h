#ifndef INC_2_SEM_LAB2_LIST_H
#define INC_2_SEM_LAB2_LIST_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

typedef struct storage_node {
    char* value;
    struct storage_node* next;
    pthread_mutex_t sync_primitive;
} storage_node_t;

typedef struct storage {
    storage_node_t* first;
    storage_node_t* last;
    int count;
    int max_count;
} storage_t;

storage_t* storage_init(int max_count);
void storage_destroy(storage_t* storage);
int storage_add(storage_t* storage, char* value);
storage_node_t* peek_in_storage_by_index(storage_t* storage, int index);
void print_storage(storage_t* storage);

#endif //INC_2_SEM_LAB2_LIST_H
