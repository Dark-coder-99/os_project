#pragma once

#include "ThreadSafeQueue.h"
#include <vector>
#include <thread>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <iostream>

class ThreadPool {
private:
    std::vector<std::thread> workers_;
    ThreadSafeQueue<std::function<void()>> task_queue_;
    size_t active_threads_count_;

public:
    // Constructor creates a specified number of worker threads.
    // Defaults to the number of hardware threads available minus one (to leave one for main thread), 
    // or at least 1 if detection fails.
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency()) {
        if (threads == 0) threads = 1; // Fallback
        active_threads_count_ = threads;

        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this] {
                // Each worker thread runs this infinite loop
                std::function<void()> task;
                
                // Keep trying to pull tasks. If wait_and_pop returns false, the pool is shutting down.
                while (task_queue_.wait_and_pop(task)) {
                    // Execute the pulled task.
                    task();
                }
            });
        }
    }

    // Enqueue a task. Accepts functions or lambdas with varying arguments.
    // Returns a std::future representing the eventual result and completion.
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type> 
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        // Package the task so we can extract its future
        auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
            
        std::future<return_type> res = task->get_future();
        
        // Ensure pool is not stopped before enqueuing a new task
        if (!task_queue_.is_valid()) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }

        // Add the task to the queue. The lambda inside captures the packaged_task pointer.
        task_queue_.push([task]() {
            (*task)();
        });

        return res;
    }

    // Get number of active worker threads
    size_t get_worker_count() const {
        return active_threads_count_;
    }

    // Destructor stops the queue and joins all threads, ensuring a clean shutdown
    ~ThreadPool() {
        task_queue_.invalidate(); // Tells workers to break the loop once queue is empty
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
};
