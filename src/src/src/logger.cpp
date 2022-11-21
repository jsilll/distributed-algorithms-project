#include "logger.hpp"

Logger::Logger(const std::string &fname, bool thread_safe)
{
    std::ios::sync_with_stdio(thread_safe);
    file_.open(fname);
}

Logger::~Logger() noexcept
{
    file_.close();
}

void Logger::Open(const std::string &fname) noexcept
{
    if (file_.is_open())
    {
        file_.close();
    }

    file_.open(fname);
}

void Logger::Flush() noexcept
{
    file_ << std::flush;
}
