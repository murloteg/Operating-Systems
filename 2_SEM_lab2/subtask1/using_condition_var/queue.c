#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>

#include "queue.h"

pthread_mutex_t mutex;
pthread_cond_t is_not_empty_queue;
pthread_cond_t is_not_full_queue;

void* qmonitor(void *arg) {
    queue_t *queue = (queue_t*) arg;
    printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

    while (true) {
        queue_print_stats(queue);
        sleep(TIME_TO_SLEEP_IN_SEC);
    }
    return NULL;
}

queue_t* queue_init(int max_count) {
    int mutex_initialization_status = pthread_mutex_init(&mutex, NULL);
    if (mutex_initialization_status != OK) {
        fprintf(stderr, "Error during initializing of mutex\n");
        abort();
    }
    int cond_var_initialization_status = pthread_cond_init(&is_not_empty_queue, NULL);
    if (cond_var_initialization_status != OK) {
        fprintf(stderr, "Error during initializing of cond_var");
        abort();
    }
    cond_var_initialization_status = pthread_cond_init(&is_not_full_queue, NULL);
    if (cond_var_initialization_status != OK) {
        fprintf(stderr, "Error during initializing of cond_var");
        abort();
    }

    queue_t* queue = (queue_t*) malloc(sizeof(queue_t));
    if (queue == NULL) {
        perror("Error during malloc()");
        abort();
    }

    queue->first = NULL;
    queue->last = NULL;
    queue->max_count = max_count;
    queue->count = 0;

    queue->add_attempts = queue->get_attempts = 0;
    queue->add_count = queue->get_count = 0;

    int creation_status = pthread_create(&queue->qmonitor_tid, NULL, qmonitor, queue);
    if (creation_status != OK) {
        fprintf(stderr, "queue_init: pthread_create() failed: with error code: %d\n", creation_status);
        abort();
    }
    return queue;
}

int queue_add(queue_t* queue, int value) {
    int lock_status = pthread_mutex_lock(&mutex);
    if (lock_status != OK) {
        fprintf(stderr, "Error during pthread_mutex_lock(); error code: %d\n", lock_status);
        return SOMETHING_WENT_WRONG;
    }

    ++queue->add_attempts;
    while (queue->count == queue->max_count) {
        pthread_cond_wait(&is_not_full_queue, &mutex);
    }

    qnode_t* new_element = (qnode_t*) malloc(sizeof(qnode_t));
    if (new_element == NULL) {
        perror("Error during malloc()");
        abort();
    }

    new_element->value = value;
    new_element->next = NULL;

    if (!queue->first) {
        queue->first = queue->last = new_element;
    } else {
        queue->last->next = new_element;
        queue->last = queue->last->next;
    }
    ++queue->count;
    ++queue->add_count;
    int signal_status = pthread_cond_signal(&is_not_empty_queue);
    if (signal_status != OK) {
        fprintf(stderr, "Error during pthread_cond_signal(); error code: %d\n", signal_status);
        return SOMETHING_WENT_WRONG;
    }
    int unlock_status = pthread_mutex_unlock(&mutex);
    if (unlock_status != OK) {
        fprintf(stderr, "Error during pthread_mutex_unlock(); error code: %d\n", unlock_status);
        return SOMETHING_WENT_WRONG;
    }
    return OK;
}

int queue_get(queue_t* queue, int* value) {
    int lock_status = pthread_mutex_lock(&mutex);
    if (lock_status != OK) {
        fprintf(stderr, "Error during pthread_mutex_lock(); error code: %d\n", lock_status);
        return SOMETHING_WENT_WRONG;
    }

    queue->get_attempts++;
    while (queue->count == 0) {
        pthread_cond_wait(&is_not_empty_queue, &mutex);
    }

    qnode_t* tmp = queue->first;
    *value = tmp->value;
    queue->first = queue->first->next;
    free(tmp);

    --queue->count;
    ++queue->get_count;
    int signal_status = pthread_cond_signal(&is_not_full_queue);
    if (signal_status != OK) {
        fprintf(stderr, "Error during pthread_cond_signal(); error code: %d\n", signal_status);
        return SOMETHING_WENT_WRONG;
    }
    int unlock_status = pthread_mutex_unlock(&mutex);
    if (unlock_status != OK) {
        fprintf(stderr, "Error during pthread_mutex_unlock(); error code: %d\n", unlock_status);
        return SOMETHING_WENT_WRONG;
    }
    return OK;
}

void queue_destroy(queue_t* queue) {
    if (queue != NULL) {
        while (queue->first != NULL) {
            qnode_t* next = queue->first->next;
            free(queue->first);
            queue->first = next;
        }
    }
    free(queue);
    int destroy_status = pthread_mutex_destroy(&mutex);
    if (destroy_status != OK) {
        fprintf(stderr, "Error during pthread_mutex_destroy(); error code: %d\n", destroy_status);
    }
    destroy_status = pthread_cond_destroy(&is_not_empty_queue);
    if (destroy_status != OK) {
        fprintf(stderr, "Error during pthread_cond_destroy(); error code: %d\n", destroy_status);
    }
    destroy_status = pthread_cond_destroy(&is_not_full_queue);
    if (destroy_status != OK) {
        fprintf(stderr, "Error during pthread_cond_destroy(); error code: %d\n", destroy_status);
    }
}

void queue_print_stats(queue_t* queue) {
    int lock_status = pthread_mutex_lock(&mutex);
    if (lock_status != OK) {
        fprintf(stderr, "Error during pthread_mutex_lock(); error code: %d\n", lock_status);
        abort();
    }
    printf("[QUEUE STATS]: [CURR SIZE: %d]; [ATTEMPTS: (%ld %ld %ld)]; [COUNTS: (%ld %ld %ld)]\n",
        queue->count,
        queue->add_attempts, queue->get_attempts, queue->add_attempts - queue->get_attempts,
        queue->add_count, queue->get_count, queue->add_count - queue->get_count);
    int unlock_status = pthread_mutex_unlock(&mutex);
    if (unlock_status != OK) {
        fprintf(stderr, "Error during pthread_mutex_unlock(); error code: %d\n", unlock_status);
        abort();
    }
}
