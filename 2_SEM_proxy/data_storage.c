#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "data_storage.h"

response_list_t* init_list() {
    response_list_t *list = (response_list_t *) calloc(1, sizeof(response_list_t));
    if (list == NULL) {
        perror("Error: calloc returned NULL");
        return NULL;
    }
    pthread_mutex_init(&list->mutex, NULL);
    list->head = NULL;
    return list;
}

void destroy_list(response_list_t* list) {
    if (list == NULL) {
        return;
    }
    pthread_mutex_destroy(&list->mutex);
    while (list->head != NULL) {
        response_node_t *tmp = list->head->next;
        free_response_record(list->head->record);
        free(list->head->record);
        free(list->head);
        list->head = tmp;
    }
    free(list);
}

void free_response_record(response_t *record) {
    if (record == NULL) {
        return;
    }
    record->valid = false;
    record->private = true;
    if (record->url != NULL) {
        free(record->url);
        record->url = NULL;
        record->URL_LEN = 0;
    }
    if (record->request != NULL) {
        free(record->request);
        record->request = NULL;
        record->request_size = 0;
    }
    if (record->response != NULL) {
        free(record->response);
        record->response = NULL;
    }
    record->response_index = 0;
    record->response_size = 0;
    pthread_mutex_lock(&record->subs_mutex);
    if (record->num_subscribers > 0) {
        record->num_subscribers = 0;
    }
    pthread_mutex_unlock(&record->subs_mutex);
    if (record->valid_subs_mutex) {
        pthread_mutex_destroy(&record->subs_mutex);
        record->valid_subs_mutex = false;
    }
    if (record->valid_rw_lock) {
        pthread_rwlock_destroy(&record->rw_lock);
        record->valid_rw_lock = false;
    }
    close(record->event_fd);
}
