#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <pthread.h>

typedef struct ThreadPool ThreadPool; //.h中暴露结构体ThreadPool

ThreadPool * threadPoolCreate(int min, int max, int queueSize);

int threadPoolDestroy(ThreadPool * pool);

void threadPoolAdd(ThreadPool *pool, void (*func)(void*), void* arg);  

int threadPoolBusyNum(ThreadPool *pool);

int threadPoolAliveNum(ThreadPool *pool);

void* worker(void* arg);

void* mananger(void* arg);

void threadExit(ThreadPool* pool);


#endif  //  _THREADPOOL_H