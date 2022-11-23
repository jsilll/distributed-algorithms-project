#pragma once

#include <unordered_set>

#include "uniform_reliable_broadcast.hpp"

class ReliableFIFOBroadcast final : public BestEffortBroadcast
{
private:
    struct PeerState
    {
        Broadcast::Message::Id::Seq next{1};
        std::set<Broadcast::Message::Id::Seq> pending; // std::set needed for in order iteration
    };

private:
    std::unordered_map<PerfectLink::Id, PeerState> peer_state_;

public:
    explicit ReliableFIFOBroadcast(Logger &logger, PerfectLink::Id id) noexcept
        : BestEffortBroadcast(logger, id, true) {}

    ~ReliableFIFOBroadcast() noexcept override = default;

protected:
    inline void SendInternal(const Broadcast::Message &msg) noexcept final
    {
        BestEffortBroadcast::SendInternal(msg);
    }

    inline void NotifyInternal(const Broadcast::Message &msg) noexcept final
    {
        BestEffortBroadcast::NotifyInternal(msg);
    }

    /**
     * @brief FIFO adds one more level of asychrony
     * Instead of logging the message right away we wait
     * for the missing messages so that we guarantee the FIFO property.
     *
     * @param id
     * @param log
     */
    void DeliverInternal(const Broadcast::Message::Id &id, bool log = false) noexcept final;
};

class UniformFIFOBroadcast final : public UniformReliableBroadcast
{
private:
    struct PeerState
    {
        Broadcast::Message::Id::Seq next{1};
        std::set<Broadcast::Message::Id::Seq> pending; // std::set for inorder iteration
    };

private:
    std::unordered_map<PerfectLink::Id, PeerState> peer_state_;

public:
    explicit UniformFIFOBroadcast(Logger &logger, PerfectLink::Id id) noexcept
        : UniformReliableBroadcast(logger, id) {}

    ~UniformFIFOBroadcast() noexcept override = default;

protected:
    inline void SendInternal(const Broadcast::Message &msg) noexcept final
    {
        UniformReliableBroadcast::SendInternal(msg);
    }

    inline void NotifyInternal(const Broadcast::Message &msg) noexcept final
    {
        UniformReliableBroadcast::NotifyInternal(msg);
    }

    /**
     * @brief FIFO adds one more level of asychrony
     * Instead of logging the message right away we wait
     * for the missing messages so that we guarantee the FIFO property.
     *
     * @param id
     * @param log
     */
    void DeliverInternal(const Broadcast::Message::Id &id, bool log = false) noexcept;
};