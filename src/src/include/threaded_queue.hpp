#pragma once

#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>

class ThreadsafeQueue
{
public:
    unsigned long size() const;
    std::optional<std::string> pop();
    void push(const std::string &item);

private:
    mutable std::mutex mutex_;
    std::queue<std::string> queue_;
};