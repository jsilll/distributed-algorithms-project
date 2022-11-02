#pragma once

#include <sstream>

#include "broadcast.hpp"

class BestEffortBroadcast : public Broadcast
{
private:
    bool log_;

public:
    explicit BestEffortBroadcast(Logger &logger, bool log = false) noexcept
        : Broadcast(logger), log_(log) {}

    ~BestEffortBroadcast() noexcept override = default;

    void Send(const std::string &msg) noexcept override;

protected:
    void Deliever(unsigned long long int sender_id, const PerfectLink::Message &msg) noexcept override;
};
