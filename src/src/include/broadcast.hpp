#pragma once

#include <string>

#include "logger.hpp"
#include "perfect_link.hpp"

class Broadcast : public PerfectLink::Manager
{
public:
    explicit Broadcast(Logger& logger) noexcept : PerfectLink::Manager::Manager(logger) {}

    ~Broadcast() noexcept override = default;

    virtual void Send(const std::string &msg) = 0;
};