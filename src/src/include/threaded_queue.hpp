#pragma once

#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>

template <typename T>
class ThreadedQueue
{
private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;

public:
    std::optional<T> Pop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty())
        {
            return std::nullopt;
        }

        std::string tmp = queue_.front();
        queue_.pop();
        return tmp;
    }

    void Push(const T &item)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(item);
    }

    // ---------- Getters ---------- //

    unsigned long size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
};