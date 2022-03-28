#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <utility>
#include <functional>
#include <stdexcept>

class ThreadPool {
public:
    inline ThreadPool(size_t threads) : stop(false)
    {
        for (size_t i = 0; i < threads; i++)
            //包裹一个完整的worker流程
            workers.emplace_back([this] {
                //循环避免虚假唤醒
                for (;;)
                {
                    std::function<void()> task;
                    // critical section for unique_lock
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                                             [this]
                                             { return this->stop || !this->tasks.empty(); });

                        if (this->stop && this->tasks.empty())
                            return;
                    
                    //出队
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            }); //结束一个worker的流程
    }

    template <typename F, typename... Args>
    auto enqueue(F &&f, Args &&...args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        //推到任务返回类型
        using return_type = typename std::result_of<F(Args...)>::type;

        //获得当前任务，无非就是类型推导，然后生成functor给packaged_task构造，这样task可以供其他人get_future
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            // std::bind返回一个基于f的函数对象，其参数被绑定到args上。
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        //获得std::future对象以供实施线程同步
        std::future<return_type> res = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop)
            {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            tasks.emplace([task]
                          { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    // the destructor joins all threads
    inline ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
        {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};
#endif