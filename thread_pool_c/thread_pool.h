#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "condition.h"

typedef struct task {
    void* arg;
    void* (*run) (void* args);
    struct task* next;          //指向queue中的下一个task
} task_t;

typedef struct thread_pool {
    condition_t ready;          //thread_pool的mutex和condition
    task_t* first;              //first task in queue
    task_t* last;               //last task in queue
    int counter;                //总线程数
    int idle;                   //空闲线程数
    int max_threads;            //最大线程数
    int quit;                   //quit flag
} threadpool_t;

void threadpool_init(threadpool_t* pool, int thread_sum);    //初始化接口，传入一个线程池对象和线程数

void threadpool_add_task(threadpool_t* pool, void* (*run)(void* args), void* arg);   //传入一个run的函数指针，以及具体的arg

void threadpool_destroy(threadpool_t* pool);

#endif
