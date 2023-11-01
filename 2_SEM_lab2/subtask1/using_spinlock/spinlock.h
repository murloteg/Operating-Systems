#ifndef INC_2_SEM_LAB2_SPINLOCK_H
#define INC_2_SEM_LAB2_SPINLOCK_H

#include <stdbool.h>

typedef struct spinlock {
    bool is_unlocked;
} spinlock_t;

void spinlock_init(spinlock_t* spinlock);
void spinlock_lock(spinlock_t* spinlock);
void spinlock_unlock(spinlock_t* spinlock);

#endif //INC_2_SEM_LAB2_SPINLOCK_H
