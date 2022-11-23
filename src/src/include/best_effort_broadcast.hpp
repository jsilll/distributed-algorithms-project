#pragma once

#include "broadcast.hpp"

/**
 * @brief
 * BEB1 Best effort validity: For any two processes pi and pj. If pi
 * and pj are correct, then every message broadcast by pi, is eventually
 * delivered by pj.
 *
 * BEB2 No duplication: No message is delivered more than once
 *
 * BEB3 No creation: If a message is delivered by some process pj, then
 * m was previously broadcast by some process pi.
 *
 */
class BestEffortBroadcast : public Broadcast
{
private:
    bool deliver_to_upper_layer_;

public:
    explicit BestEffortBroadcast(Logger &logger, PerfectLink::Id id, bool deliver_to_upper_layer = false) noexcept
        : Broadcast(logger, id), deliver_to_upper_layer_(deliver_to_upper_layer) {}

    ~BestEffortBroadcast() noexcept override = default;

protected:
    void SendInternal(const Broadcast::Message &msg) noexcept override;

    void NotifyInternal(const Broadcast::Message &msg) noexcept override;
    
    inline void DeliverInternal(const Broadcast::Message::Id &id, bool log = false) noexcept override
    {
        if (log)
        {
            LogDeliver(id);
        }
    }
};
