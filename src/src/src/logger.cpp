#include "logger.hpp"

Logger::Logger(const std::string &fname)
{
    file.open(fname);
}

Logger::~Logger()
{
    if (file.is_open())
    {
        file.close();
    }
}

void Logger::open(const std::string &fname)
{
    if (file.is_open())
    {
        file.close();
    }

    file.open(fname);
}

void Logger::flush()
{
    file << std::flush;
}
