#include "condition.h"

int condition_init(condition_t* cond) {
    int status;
    status = pthread_mutex_init(&cond->mutex, NULL);     //pthread_mutex_init第一个参数是pthread_mutex_t的地址，第二个参数是mutex的attr
    if (status != 0) {
        printf("Error: pthread_mutex_init failed, return value:%d\n", status);
        return status;
    }
    status = pthread_cond_init(&cond->condition, NULL);
    if (status != 0) {
       printf("Error: pthread_cond_init failed, return value:%d\n", status);
       return status;
    }
    return 0;
}

int condition_lock(condition_t* cond) {
    return pthread_mutex_lock(&cond->mutex);
}

int condition_unlock(condition_t* cond) {
    return pthread_mutex_unlock(&cond->mutex);
}

//条件等待
int condition_wait(condition_t* cond) {
    return pthread_cond_wait(&cond->condition, &cond->mutex);       //pthread_cond_t, 第一个参数是条件变量的地址， 第二个参数是对应的mutex，mutex在wait前应该上锁
}

//激活一个条件等待的线程
int condition_signal(condition_t* cond) {
    return pthread_cond_signal(&cond->condition);
}

//唤醒所有线程
int condition_broadcast(condition_t* cond) {
    return pthread_cond_broadcast(&cond->condition);
}

int condition_destroy(condition_t* cond) {
    int status;
    status = pthread_mutex_destroy(&cond->mutex);
     if (status != 0) {
        return status;
    }
    status = pthread_cond_destroy(&cond->condition);
    if (status != 0) {
        return status;
    }
    return 0;
}

int condition_timedwait(condition_t* cond, struct timespec* abs_time) {
    return pthread_cond_timedwait(&cond->condition, &cond->mutex, abs_time);
}