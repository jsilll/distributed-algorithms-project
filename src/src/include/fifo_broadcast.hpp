#pragma once

#include "uniform_reliable_broadcast.hpp"

/**
 * @brief
 *
 */
class FIFOBroadcast : public UniformReliableBroadcast
{
public:
    explicit FIFOBroadcast(Logger &logger, unsigned long long id) noexcept
        : UniformReliableBroadcast(logger, id) {}

    ~FIFOBroadcast() noexcept override = default;

protected:
    void SendInternal(const Broadcast::Message &msg) noexcept override
    {
        UniformReliableBroadcast::SendInternal(msg);
    }

    void NotifyInternal(const Broadcast::Message &msg) noexcept override
    {
        UniformReliableBroadcast::NotifyInternal(msg);
    }

    /**
     * @brief FIFO adds one more level of asychrony
     * Instead of logging the message we are going to wait
     * for other messages so that we can guarantee the FIFO property.
     * 
     * @param id 
     * @param log 
     */
    void DeliverInternal(const Broadcast::Message::Id &id, bool log = false) noexcept override
    {
        if (log)
        {
            LogDeliver(id);
        }
    }
};