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
    void SendInternal(const Broadcast::Message &msg) noexcept override
    {
        std::vector<PerfectLink *> pls;

        perfect_links_.mutex.lock_shared();
        pls.reserve(perfect_links_.data.size());
        for (const auto &[_, pl] : perfect_links_.data)
        {
            pls.emplace_back(pl.get());
        }
        perfect_links_.mutex.unlock_shared();

        static_assert(UDPServer::kMaxSendSize > PerfectLink::kPacketPrefixSize);
        static_assert((UDPServer::kMaxSendSize - PerfectLink::kPacketPrefixSize) > kPacketPrefixSize);

        char buffer[UDPServer::kMaxSendSize - PerfectLink::kPacketPrefixSize];
        std::size_t len = Serialize(msg, buffer);

#ifdef DEBUG
        if (msg.id.author == id_)
        {
            std::cout << "[DBUG] Best Effort Broadcast sending message " << msg.id.seq << " of size: " << len << "\n";
        }
#endif

        for (const auto pl : pls)
        {
            pl->Send(buffer, len);
        }
    }

    void NotifyInternal(const Broadcast::Message &msg) noexcept override
    {
        if (deliver_to_upper_layer_)
        {
            DeliverInternal(msg.id, true);
        }
        else
        {
            BestEffortBroadcast::DeliverInternal(msg.id);
        }
    }

    void DeliverInternal(const Broadcast::Message::Id &id, bool log = false) noexcept override
    {
        if (log)
        {
            LogDeliver(id);
        }
    }
};
