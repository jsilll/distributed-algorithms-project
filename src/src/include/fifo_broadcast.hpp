#pragma once

#include <unordered_set>

#include "uniform_reliable_broadcast.hpp"

/**
 * @brief
 *
 */
class FIFOBroadcast final : public UniformReliableBroadcast
{
private:
    struct PeerState {
        Broadcast::Message::Id next;
        std::unordered_set<Broadcast::Message::Id> pending;
    };

private:
    std::unordered_map<PerfectLink::Id, PeerState> peer_state_;

public:
    explicit FIFOBroadcast(Logger &logger, unsigned long long id) noexcept
        : UniformReliableBroadcast(logger, id) {}

    ~FIFOBroadcast() noexcept override = default;

protected:
    void SendInternal(const Broadcast::Message &msg) noexcept final
    {
        UniformReliableBroadcast::SendInternal(msg);
    }

    void NotifyInternal(const Broadcast::Message &msg) noexcept final
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
    void DeliverInternal(const Broadcast::Message::Id &id, bool log = false) noexcept final
    {
        if (log)
        {
            LogDeliver(id);
        }
    }
};