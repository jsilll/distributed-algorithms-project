#include "best_effort_broadcast.hpp"

#include <cassert>

void BestEffortBroadcast::SendInternal(const Broadcast::Message &msg) noexcept
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

    std::vector<char> buffer(kPacketPrefixSize + msg.payload.size());
    std::size_t len = Serialize(msg, buffer);

#ifdef DEBUG
    if (msg.id.author == id_)
    {
        std::cout << "[DBUG] Best Effort Broadcast sending message of size: " << msg.payload.size() << "\n";
    }
#endif

    for (const auto pl : pls)
    {
        pl->Send(buffer);
    }
}

void BestEffortBroadcast::SendDirectedInternal(const Broadcast::Message &msg, PerfectLink::Id target) noexcept
{
    static_assert(UDPServer::kMaxSendSize > PerfectLink::kPacketPrefixSize);
    static_assert((UDPServer::kMaxSendSize - PerfectLink::kPacketPrefixSize) > kPacketPrefixSize);

    std::vector<char> buffer(kPacketPrefixSize + msg.payload.size());
    std::size_t len = Serialize(msg, buffer);

#ifdef DEBUG
    if (msg.id.author == id_)
    {
        std::cout << "[DBUG] Best Effort Broadcast sending directed message to " << target << " of size: " << msg.payload.size() << "\n";
    }
#endif

    perfect_links_.mutex.lock_shared();
    if (perfect_links_.data.count(target))
    {
        perfect_links_.data.at(target)->Send(buffer);
    }
    perfect_links_.mutex.unlock_shared();
}

void BestEffortBroadcast::NotifyInternal(const Broadcast::Message &msg) noexcept
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
