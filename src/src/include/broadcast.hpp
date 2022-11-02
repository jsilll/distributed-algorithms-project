#pragma once

#include <string>

#include "logger.hpp"
#include "perfect_link.hpp"

class Broadcast : public PerfectLink::Manager
{
private:
    bool log_;

public:
    Broadcast(Logger& logger, bool log = false) noexcept : PerfectLink::Manager::Manager(logger), log_(log) {}

    virtual ~Broadcast() noexcept override = default;

    virtual void Send(const std::string &msg) = 0;
};