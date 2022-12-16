#pragma once

#include <string>
#include <fstream>
#include <iostream>

class Logger
{
private:
    std::ofstream file_;

public:
    explicit Logger(const std::string &fname, bool thread_safe = true);

    ~Logger() noexcept;

    Logger(const Logger &) = delete;
    Logger(const Logger &&) = delete;

    Logger &operator=(const Logger &) = delete;
    Logger &operator=(const Logger &&) = delete;

    void Flush() noexcept;

    void Open(const std::string &fname) noexcept;

    inline friend Logger &operator<<(Logger &logger, const std::string &text) noexcept
    {
        std::string text_w_nl = text + "\n";
        logger.file_ << text_w_nl;
#ifdef DEBUG
        std::cout << "[DLOG] " << text_w_nl;
#endif
        return logger;
    }
};