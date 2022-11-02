#pragma once

#include <sstream>

#include "broadcast.hpp"

class BestEffortBroadcast : public Broadcast
{
private:
    bool log_;

public:
    BestEffortBroadcast(Logger &logger, bool log = false) noexcept : Broadcast(logger), log_(log) {}

    virtual ~BestEffortBroadcast() noexcept override = default;

    virtual void Send(const std::string &msg) noexcept override
    {
        PerfectLink::Message::Id id{};

        perfect_links_.mutex.lock_shared();
        for (const auto &[pid, pl] : perfect_links_.data)
        {
            id = pl->Send(msg);
        }
        perfect_links_.mutex.unlock_shared();

        if (log_)
        {
            std::stringstream ss;
            ss << "b " << id;
            logger_ << ss.str();
        }
    }

protected:
    virtual void Deliever(unsigned long long int sender_id, const PerfectLink::Message &msg) noexcept override
    {
        if (log_)
        {
            std::stringstream ss;
            ss << "d " << sender_id << " " << msg.id;
            logger_ << ss.str();
        }
    }
};