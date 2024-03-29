#ifndef __FITOS_QUEUE_H__
#define __FITOS_QUEUE_H__

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

typedef struct queue_node {
	int value;
	struct queue_node* next;
} qnode_t;

typedef struct queue {
	qnode_t* first;
	qnode_t* last;

	pthread_t qmonitor_tid;

	int count;
	int max_count;

	// queue statistics
	long add_attempts;
	long get_attempts;
	long add_count;
	long get_count;
} queue_t;

queue_t* queue_init(int max_count);
void queue_destroy(queue_t* q);
int queue_add(queue_t* q, int value);
int queue_get(queue_t* q, int* value);
void queue_print_stats(queue_t* q);

#endif		// __FITOS_QUEUE_H__
