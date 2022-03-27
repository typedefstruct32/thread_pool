#include <stdlib.h>
#include <errno.h>

#include "thread_pool.h"


void* thread_routine(void* arg); //前置声明

void threadpool_init(threadpool_t* pool, int threads_sum) {
    int n_status = condition_init(&pool->ready);
    if (n_status == 0) {
        printf("Info: initialize the thread pool successfully!\n");
    } else {
        printf("Info: initialize the thread pool failed, status:%d\n", n_status);
    }
    pool->first = NULL;
    pool->last = NULL;
    pool->idle = 0;
    pool->counter = 0;
    pool->max_threads = threads_sum;
    pool->quit = 0;
}


void threadpool_add_task(threadpool_t* pool, void* (*run)(void* arg), void* arg) {
    //create a new task
    task_t* new_task = (task_t*)malloc(sizeof(task_t));
    new_task->run = run;
    new_task->arg = arg;
    new_task->next = NULL;
    
    //lock the condition
    condition_lock(&pool->ready);
    
    //add the task to queue
    if (pool->first == NULL) 
        pool->first = new_task;
    else {
        pool->last->next = new_task;
    }
    pool->last = new_task;

    //当将task入队后，需要将其分配到一个线程运行，如果idle > 0,则唤醒一个thread去执行task。如果idle == 0 && thread_num < MAX_THREAD_NUM则创建线程
    if (pool->idle > 0) {
        //awake a thread
        condition_signal(&pool->ready);
    } else if (pool->counter < pool->max_threads) {
        //创建线程

        pthread_t tid;
        /*
        * pthread_create():
         * (1)thread identifier
         * (2)set the pthread attribute
         * (3)the function that thread is going to run
         * (4)the args of run func
        */
        pthread_create(&tid, NULL, thread_routine, pool);
        pool->counter++;
    } else {
        printf("Warning: no idle thread, please wait...\n");
    }
    //unlock the conditon
    condition_unlock(&pool->ready);
}

void* thread_routine(void* arg) {
    struct timespec abs_time;
    int timeout;
    printf("Info: create thread, and the thread id is: %ld\n", pthread_self());
    threadpool_t * pool = (threadpool_t*)(arg);
    
    //轮询task queue
    while (1) {
        timeout = 0;
        condition_lock(&pool->ready);
        //先对pool->idle++是必要的，触发外层循环的唤醒条件
        pool->idle++;
        
        while (pool->first == NULL && !pool->quit) {
            printf("Info: thread %ld is waiting for a task\n", pthread_self());
            // get the system time
            clock_gettime(CLOCK_REALTIME, &abs_time);
            abs_time.tv_sec += 2;
            int status;
            status = condition_timedwait(&pool->ready, &abs_time);
            if (status == ETIMEDOUT) {
                printf("Info: thread %ld wait timed out\n", pthread_self());
                timeout = 1;
                break;
            }
        }

        pool->idle--;
        if (pool->first != NULL) {
            //get the task from queue
            task_t* t = pool->first;
            pool->first = t->next;
            
            //对pool的读写已经完毕，需要释放锁
            condition_unlock(&pool->ready);

            // run the task
            t->run(t->arg);
            free(t);
        }

         //尝试再次获取锁用来做pool的状态检查
        condition_lock(&pool->ready);
        // 检查是否task队列已经空了，且pool已经进入退出流程，此时直接break，线程结束并销毁
        if (pool->quit && pool->first == NULL) {
            pool->counter--;
            // 若线程池中线程数为0，通知等待线程（主线程）全部任务已经完成
            if (pool->counter == 0) {
                condition_signal(&pool->ready);
            }
            condition_unlock(&pool->ready);
            break;  // destroy the thread
        }

        // if visit task queue timeout(means no task in queue), quit destory the thread
        if (timeout) {
            pool->counter--;
            condition_unlock(&pool->ready);
            break;  // destroy the thread
        }

        condition_unlock(&pool->ready);
    }

    // if break, destroy the thread
    printf("Info: thread %ld quit\n", pthread_self());
    return NULL;        
}

void threadpool_destroy(threadpool_t *pool) {
    if (pool->quit)
        return;
    condition_lock(&pool->ready);
    pool->quit = 1;
    if (pool->counter > 0) {
        if (pool->idle > 0) {
            condition_broadcast(&pool->ready);
        }
        while (pool->counter > 0) {
            condition_wait(&pool->ready);
        }
    }
    condition_unlock(&pool->ready);
    condition_destroy(&pool->ready);
}