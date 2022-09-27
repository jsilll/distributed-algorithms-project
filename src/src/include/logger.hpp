#pragma once

#include <fstream>

class Logger
{

public:
    Logger(const std::string &fname = "output.txt");

    ~Logger();

    Logger(const Logger &) = delete;

    Logger &operator=(const Logger &) = delete;

    void open(const std::string &fname);

    void flush();

    inline friend Logger &operator<<(Logger &logger, const std::string &text)
    {
        logger.file << text << std::endl;
        return logger;
    }

private:
    std::ofstream file;
};