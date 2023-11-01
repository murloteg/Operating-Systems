#define _GNU_SOURCE
#include <stdatomic.h>

#include "spinlock.h"

void spinlock_init(spinlock_t* spinlock) {
    spinlock->is_unlocked = true;
}

void spinlock_lock(spinlock_t* spinlock) {
    while (true) {
        bool expected = true;
        if (atomic_compare_exchange_strong(&spinlock->is_unlocked, &expected, false)) {
            break;
        }
    }
}

void spinlock_unlock(spinlock_t* spinlock) {
    bool expected = false;
    atomic_compare_exchange_strong(&spinlock->is_unlocked, &expected, true);
}
