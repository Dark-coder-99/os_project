#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cond_var_;
    bool valid_{true}; // Flag to indicate if the queue is still accepting/processing items

public:
    ThreadSafeQueue() = default;

    // Delete copy constructors to prevent accidental copying of locks
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    ~ThreadSafeQueue() {
        invalidate();
    }

    // Pushes an item to the end of the queue
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!valid_) return; // Don't push if invalidated
            queue_.push(std::move(value));
        }
        cond_var_.notify_one(); // Wake up one waiting thread
    }

    // Tries to pop an item without waiting. Returns true on success.
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty() || !valid_) {
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    // Blocks until an item is available, or the queue gets invalidated
    bool wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_var_.wait(lock, [this]() { 
            return !queue_.empty() || !valid_; 
        });

        // If queue was invalidated and is empty, stop waiting
        if (!valid_ && queue_.empty()) {
            return false;
        }

        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    // Checking empty state
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    // Gets current size
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    // Invalidates the queue, waking up all waiting threads
    void invalidate() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            valid_ = false;
        }
        cond_var_.notify_all();
    }
    
    // Check validity
    bool is_valid() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return valid_;
    }
};
