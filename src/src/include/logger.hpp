#pragma once

#include <string>
#include <fstream>
#include <iostream>

class Logger
{
private:
    bool debug_{};
    std::ofstream file_;

public:
    Logger(const std::string &fname = "output.txt");

    Logger(const Logger &) = delete;

    Logger &operator=(const Logger &) = delete;

    void Flush();

    void Open(const std::string &fname);

    inline friend Logger &operator<<(Logger &logger, const std::string &text)
    {
        logger.file_ << text << std::endl;

        if (logger.debug_)
        {
            std::cout << text << "\n";
        }

        return logger;
    }
};