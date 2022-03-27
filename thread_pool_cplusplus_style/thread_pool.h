#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stdio.h>
#include <pthread.h>
#include <functional>
#include <vector>
#include <queue>

class ThreadPool {
public:
    //定义函数指针
    typedef void* (WorkerFunc)(void* arg);  
    //定义task结构体

    struct Task {
        WorkerFunc* func;
        void* arg;
    };

    //constructor
    explicit ThreadPool(int thread_num);
    ~ThreadPool();
    
    //addTask接口
    void addTask(WorkerFunc* func, void* arg);
    
private:
    //task queue
    std::queue<Task*> task_queue_;
    //thread_list
    std::vector<pthread_t> thread_list_;
    bool is_running_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;

    //thread_routine for run threaad
    static void* thread_routine(void* pool_ptr);
    void thread_worker();
};


//implementation

inline ThreadPool::ThreadPool(int thread_num) : is_running_(true) {
    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_, NULL);
    
    for (int i = 0; i < thread_num; i++) {
        pthread_t pid;
        //创建线程，this指针带到thread中去当作参数.
        pthread_create(&pid, NULL, thread_routine, this);
        thread_list_.push_back(pid);
    }
}

inline ThreadPool::~ThreadPool() {
    pthread_mutex_lock(&mutex_);
    is_running_ = false;
    pthread_mutex_unlock(&mutex_);
    //唤醒所有还在等待task的线程
    pthread_cond_broadcast(&cond_);
    //等待这些线程完成
    for (int i = 0; i < thread_list_.size(); i++)
        pthread_join(thread_list_[i], NULL);
    pthread_cond_destroy(&cond_);
    pthread_mutex_destroy(&mutex_);
}

inline void ThreadPool::addTask(WorkerFunc* func, void* arg) {
    Task* task = new Task();
    task->func = func;
    task->arg = arg;

    pthread_mutex_lock(&mutex_);
    task_queue_.push(task);
    pthread_mutex_unlock(&mutex_);
    //唤醒在等待的thread
    pthread_cond_signal(&cond_);
}

//执行线程的routine，主要是将thread_worker包装成给pthread_create调用的形式

inline void* ThreadPool::thread_routine(void* pool_ptr) {
    ThreadPool* pool = static_cast<ThreadPool*>(pool_ptr);
    pool->thread_worker();
}


inline void ThreadPool::thread_worker() {
    Task* task = NULL;
    while(true) {
        pthread_mutex_lock(&mutex_);
        //检查当前pool是否在running，否则进入线程结束流程。
        if (!is_running_) {
            pthread_mutex_unlock(&mutex_);
            break;
        }
        while (task_queue_.empty()) {
            //等待喂养task
            pthread_cond_wait(&cond_, &mutex_);
        }

        // if (task_queue_.empty()) {
        //     pthread_cond_wait(&cond_, &mutex_);  // 获取不到任务时阻塞, 直到有新的任务入队
        //     if (task_queue_.empty()) {
        //         pthread_mutex_unlock(&mutex_);
        //         continue;
        //     }
        // }
        printf("threads %d working\n", pthread_self());
        task = task_queue_.front();
        task_queue_.pop();
        pthread_mutex_unlock(&mutex_);
        //执行task
        (*(task->func))(task->arg);
        task = NULL; 
        delete task;
    }

    //线程池停止时，进入这里.需要确保任务队列为空时才退出(防止tasks内存泄露)
    while (true) {
        pthread_mutex_lock(&mutex_);
        if (task_queue_.empty()) {
            pthread_mutex_unlock(&mutex_);
            break;
        }
        task = task_queue_.front();
        task_queue_.pop();
        pthread_mutex_unlock(&mutex_);
        task = NULL;
        delete task;
    }

    printf("Info: thread[%lu] exit\n", pthread_self());
}

#endif