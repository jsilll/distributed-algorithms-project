#include "fifo_broadcast.hpp"

void ReliableFIFOBroadcast::DeliverInternal(const Broadcast::Message::Id &id, bool log) noexcept
{
    auto &state = peer_state_[id.author];

    if (id.seq < state.next)
    {
        return;
    }

    std::vector<Message::Id::Seq> to_remove;
    to_remove.reserve(state.pending.size());

    state.pending.insert(id.seq);
    for (const auto seq : state.pending)
    {
        if (seq > state.next)
        {
            break;
        }
        else
        {
            if (seq > state.next)
            {
                break;
            }

            to_remove.emplace_back(seq);

            if (log)
            {
                LogDeliver({seq, id.author});
            }

            state.next++;
        }
    }

    for (const auto seq : to_remove)
    {
        state.pending.erase(seq);
    }
}

void UniformFIFOBroadcast::DeliverInternal(const Broadcast::Message::Id &id, bool log) noexcept 
{
    auto &state = peer_state_[id.author];

    if (id.seq < state.next)
    {
        return;
    }

    std::vector<Message::Id::Seq> to_remove;
    to_remove.reserve(state.pending.size());

    state.pending.insert(id.seq);
    for (const auto seq : state.pending)
    {
        if (seq > state.next)
        {
            break;
        }

        to_remove.emplace_back(seq);

        if (log)
        {
            LogDeliver({seq, id.author});
        }

        state.next++;
    }

    for (const auto seq : to_remove)
    {
        state.pending.erase(seq);
    }
}