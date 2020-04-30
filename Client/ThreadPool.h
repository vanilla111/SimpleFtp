//
// Created by wang on 2019/12/17.
//

#ifndef DNS_THREADPOOL_H
#define DNS_THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <mutex>

class ThreadPool {
public:
    ThreadPool(size_t);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~ThreadPool();
private:
    // 工作者
    std::vector< std::thread > workers;
    // 任务队列
    std::queue< std::function<void()> > tasks;

    // 异步信号
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// 初始化一些工作者
inline ThreadPool::ThreadPool(size_t threads) : stop(false)
{
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back([this] {
                    for(;;)
                    {
                        std::function<void()> task;

                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            this->condition.wait(lock,[this]{
                                return this->stop || !this->tasks.empty();
                            });
                            if(this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }

                        task();
                    }
                }
        );
}

// 新增工作到池中
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        if(stop)
            throw std::runtime_error("停止线程池后不允许新增工作到池中");

        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

#endif //DNS_THREADPOOL_H
