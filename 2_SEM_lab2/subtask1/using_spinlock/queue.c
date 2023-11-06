#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>

#include "queue.h"

pthread_spinlock_t spinlock;

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
    int spinlock_initialization_status = pthread_spin_init(&spinlock, PTHREAD_PROCESS_SHARED);
    if (spinlock_initialization_status != OK) {
        fprintf(stderr, "Error during initializing of spinlock\n");
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
    int lock_status = pthread_spin_lock(&spinlock);
    if (lock_status != OK) {
        fprintf(stderr, "Error during pthread_spin_lock(); error code: %d\n", lock_status);
        return SOMETHING_WENT_WRONG;
    }
    ++queue->add_attempts;
    assert(queue->count <= queue->max_count);
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
    int unlock_status = pthread_spin_unlock(&spinlock);
    if (unlock_status != OK) {
        fprintf(stderr, "Error during pthread_spin_unlock(); error code: %d\n", unlock_status);
        return SOMETHING_WENT_WRONG;
    }
    return OK;
}

int queue_get(queue_t* queue, int* value) {
    int lock_status = pthread_spin_lock(&spinlock);
    if (lock_status != OK) {
        fprintf(stderr, "Error during pthread_spin_lock(); error code: %d\n", lock_status);
        return SOMETHING_WENT_WRONG;
    }
    queue->get_attempts++;
    assert(queue->count >= 0);
    if (queue->count == 0) {
        return SOMETHING_WENT_WRONG;
    }

    qnode_t* tmp = queue->first;
    *value = tmp->value;
    queue->first = queue->first->next;
    free(tmp);

    --queue->count;
    ++queue->get_count;
    int unlock_status = pthread_spin_unlock(&spinlock);
    if (unlock_status != OK) {
        fprintf(stderr, "Error during pthread_spin_unlock(); error code: %d\n", unlock_status);
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
    int destroy_status = pthread_spin_destroy(&spinlock);
    if (destroy_status != OK) {
        fprintf(stderr, "Error during pthread_spin_destroy(); error code: %d\n", destroy_status);
    }
}

void queue_print_stats(queue_t* queue) {
    int lock_status = pthread_spin_lock(&spinlock);
    if (lock_status != OK) {
        fprintf(stderr, "Error during pthread_spin_lock(); error code: %d\n", lock_status);
        return SOMETHING_WENT_WRONG;
    }
    printf("[QUEUE STATS]: [CURR SIZE: %d]; [ATTEMPTS: (%ld %ld %ld)]; [COUNTS: (%ld %ld %ld)]\n",
        queue->count,
        queue->add_attempts, queue->get_attempts, queue->add_attempts - queue->get_attempts,
        queue->add_count, queue->get_count, queue->add_count - queue->get_count);
    if (unlock_status != OK) {
        fprintf(stderr, "Error during pthread_spin_unlock(); error code: %d\n", unlock_status);
        return SOMETHING_WENT_WRONG;
    }
}
