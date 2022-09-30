#include "logger.hpp"

Logger::Logger(const std::string &fname)
{
    file_.open(fname);
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
    file_ << std::flush;
}
