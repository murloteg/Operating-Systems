#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>

#include "queue.h"

spinlock_t spinlock;

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
    ++queue->add_attempts;
    assert(queue->count <= queue->max_count);

    spinlock_lock(&spinlock);
    if (queue->count == queue->max_count) {
        return SOMETHING_WENT_WRONG;
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
    spinlock_unlock(&spinlock);
    return OK;
}

int queue_get(queue_t* queue, int* value) {
    queue->get_attempts++;
    assert(queue->count >= 0);

    spinlock_lock(&spinlock);
    if (queue->count == 0) {
        return SOMETHING_WENT_WRONG;
    }

    qnode_t* tmp = queue->first;
    *value = tmp->value;
    queue->first = queue->first->next;
    free(tmp);

    --queue->count;
    ++queue->get_count;
    spinlock_unlock(&spinlock);
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
}

void queue_print_stats(queue_t* queue) {
    printf("[QUEUE STATS]: [CURR SIZE: %d]; [ATTEMPTS: (%ld %ld %ld)]; [COUNTS: (%ld %ld %ld)]\n",
        queue->count,
        queue->add_attempts, queue->get_attempts, queue->add_attempts - queue->get_attempts,
        queue->add_count, queue->get_count, queue->add_count - queue->get_count);
}
