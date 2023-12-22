#ifndef HTTP_PROXY_DATA_STORAGE_H
#define HTTP_PROXY_DATA_STORAGE_H
#include <pthread.h>
#include <stdbool.h>

typedef struct data_storage {
    char *url;
    char *request;
    char *response;
    pthread_rwlock_t rw_lock;
    pthread_mutex_t subs_mutex;
    size_t URL_LEN;
    size_t request_size;
    size_t response_size;
    size_t response_index;
    int num_subscribers;
    int event_fd;
    bool server_alive;
    bool full;
    bool valid;
    bool private;
    bool valid_rw_lock;
    bool valid_subs_mutex;
} response_t;

typedef struct response_node_t {
    response_t *record;
    struct response_node_t *next;
} response_node_t;

typedef struct list_t {
    struct response_node_t *head;
    pthread_mutex_t mutex;
} response_list_t;

response_list_t* init_list();
void destroy_list(response_list_t *list);
void free_response_record(response_t *record);

#endif //HTTP_PROXY_DATA_STORAGE_H
