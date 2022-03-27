#ifndef CONDITION_H
#define CONDITION_H

#include <stdio.h>
#include <pthread.h>

typedef struct condition {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} condition_t;

int condition_init(condition_t* cond);

int condition_lock(condition_t* cond);

int condition_unlock(condition_t* cond);

int condition_signal(condition_t* cond);

int condition_broadcast(condition_t* cond);

int condition_destroy(condition_t* cond);

int condition_wait(condition_t* cond);

int condition_timedwait(condition_t* cond, struct timespec* abs_name);

#endif