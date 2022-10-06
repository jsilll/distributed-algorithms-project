#pragma once

#include <mutex>
#include <string>
#include <fstream>
#include <iostream>

class Logger
{
private:
    std::mutex mutex_;
    std::ofstream file_;

public:
    explicit Logger(const std::string &fname = "output.txt");

    Logger(const Logger &) = delete;

    ~Logger();

    Logger &operator=(const Logger &) = delete;

    void Flush();

    void Open(const std::string &fname);

    std::mutex& mutex() 
    {
        return mutex_;
    }

    inline friend Logger &operator<<(Logger &logger, const std::string &text)
    {
        logger.mutex().lock();
        logger.file_ << text << std::endl;
#ifdef DEBUG
        std::cout << "[DLOG] " << text << "\n";
#endif
        logger.mutex().unlock();
        return logger;
    }
};