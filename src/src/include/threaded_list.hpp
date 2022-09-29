#pragma once

#include <iostream>
#include <mutex>
#include <list>
#include <string>
#include <atomic>

class ThreadsafeList
{
public:
    unsigned long size() const;
    std::atomic<bool> contains(const std::string &item) const;
    void push_back(const std::string &item);
    std::list<std::string> getList() const;

private:
    std::mutex mutex_;
    std::list<std::string> list_;
};