#include "threaded_queue.hpp"

#include <mutex>
#include <optional>
#include <queue>
#include <string>

unsigned long ThreadsafeQueue::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

std::optional<std::string> ThreadsafeQueue::pop()
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

void ThreadsafeQueue::push(const std::string &item)
{
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(item);
}
