#include "logger.hpp"

Logger::Logger(const std::string &fname)
{
    file_.open(fname);
}

Logger::~Logger()
{   
    mutex_.lock();
    file_.close();
    mutex_.unlock();
}

void Logger::Open(const std::string &fname)
{
    if (file_.is_open())
    {
        file_.close();
    }

    file_.open(fname);
}

void Logger::Flush()
{
    mutex_.lock();
    file_ << std::flush;
    mutex_.unlock();
}
