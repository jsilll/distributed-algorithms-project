#include "uniform_reliable_broadcast.hpp"

void UniformReliableBroadcast::DeliveredSet::PeerState::Insert(Broadcast::Message::Id::Seq seq) noexcept
{
    if (seq == bottom_)
    {
        bottom_++;

        std::vector<Broadcast::Message::Id::Seq> to_remove;
        to_remove.reserve(delivered_.size());

        for (const auto seq : delivered_)
        {
            if (seq == bottom_)
            {
                to_remove.emplace_back(seq);
                bottom_++;
            }
            else
            {
                break;
            }
        }

        for (const auto seq : to_remove)
        {
            delivered_.erase(seq);
        }
    }
    else
    {
        delivered_.insert(seq);
    }
}

std::size_t UniformReliableBroadcast::DeliveredSet::Size() const noexcept
{
    std::size_t res = 0;
    for (const auto &[_, peer_state] : state_)
    {
        res += peer_state.Size();
    }
    return res;
}

void UniformReliableBroadcast::Add(std::unique_ptr<PerfectLink> pl) noexcept
{
    const PerfectLink::Id id = pl->target_id();

    perfect_links_.mutex.lock();
    perfect_links_.data[id] = std::move(pl);
    perfect_links_.data[id]->Subscribe(this);
    perfect_links_.mutex.unlock();

    n_processes_.fetch_add(1);
    n_own_pending_delivery_ideal_.store(static_cast<unsigned int>(std::max(1, static_cast<int>(URB_MAX_MSGS_IN_NETWORK / std::pow(n_processes_.load(), 2)))));
}

void UniformReliableBroadcast::Send(const std::string &msg) noexcept
{
    Broadcast::Message::Id::Seq seq = n_messages_sent_.fetch_add(1);
    Broadcast::Message message = {{seq, id_}, id_, {msg.begin(), msg.end()}};

    LogSend(seq);

    if (n_own_pending_for_delivery_.load() >= n_own_pending_delivery_ideal_.load())
    {
        own_pending_for_broadcast_.mutex.lock_shared();
        own_pending_for_broadcast_.data.push(message);
        own_pending_for_broadcast_.mutex.unlock_shared();
        return;
    }

    n_own_pending_for_delivery_.fetch_add(1);
    SendInternal(message);
}

void UniformReliableBroadcast::NotifyInternal(const Broadcast::Message &msg) noexcept
{
    BestEffortBroadcast::NotifyInternal(msg);

    pending_for_delivery_.mutex.lock_shared();
    bool not_pending = pending_for_delivery_.data.count(msg.id) == 0;
    pending_for_delivery_.mutex.unlock_shared();

    delivered_.mutex.lock_shared();
    bool not_delivered = !delivered_.data.Contains(msg.id);
    delivered_.mutex.unlock_shared();

    if (not_delivered)
    {
        acks_.mutex.lock();
        acks_.data[msg.id].insert(msg.sender);
        acks_.mutex.unlock();

        if (not_pending)
        {
            pending_for_delivery_.mutex.lock();
            pending_for_delivery_.data.insert(msg.id);
            pending_for_delivery_.mutex.unlock();
#ifdef DEBUG
            std::cout << "[DBUG] URB Relaying: " << msg.id.author << " " << msg.id.seq << "\n";
#endif
            BestEffortBroadcast::SendInternal(msg);
        }
    }
}

void UniformReliableBroadcast::DeliverPending() noexcept
    {
        while (on_.load())
        {
            pending_for_delivery_.mutex.lock();
            std::vector<Broadcast::Message::Id> pending_messages(pending_for_delivery_.data.begin(), pending_for_delivery_.data.end());
#ifdef DEBUG
            std::cout << "[DBUG] URB Pending: " << pending_for_delivery_.data.size() << "\n";
#endif
            pending_for_delivery_.mutex.unlock();

            for (const auto &id : pending_messages)
            {
                acks_.mutex.lock();
                bool majority_seen = (acks_.data[id].size() + 1) > static_cast<std::size_t>(std::floor(n_processes_.load() / 2));
                acks_.mutex.unlock();

                delivered_.mutex.lock_shared();
                bool not_delivered = !delivered_.data.Contains(id);
                delivered_.mutex.unlock_shared();

                if (majority_seen && not_delivered)
                {
                    pending_for_delivery_.mutex.lock();
                    pending_for_delivery_.data.erase(id);
                    pending_for_delivery_.mutex.unlock();

                    if (id.author == id_ && n_own_pending_for_delivery_.fetch_add(static_cast<unsigned int>(-1)) <= n_own_pending_delivery_ideal_.load())
                    {
                        own_pending_for_broadcast_.mutex.lock();
                        bool pending_for_broadcast_not_empty = !own_pending_for_broadcast_.data.empty();
                        auto &msg = own_pending_for_broadcast_.data.front();
                        if (pending_for_broadcast_not_empty)
                        {
                            own_pending_for_broadcast_.data.pop();
                        }
                        own_pending_for_broadcast_.mutex.unlock();

                        if (pending_for_broadcast_not_empty)
                        {
                            n_own_pending_for_delivery_.fetch_add(1);
                            SendInternal(msg);
                        }
                    }

                    delivered_.mutex.lock();
                    delivered_.data.Insert(id);
                    delivered_.mutex.unlock();

                    acks_.mutex.lock();
                    acks_.data.erase(id);
                    acks_.mutex.unlock();
#ifdef DEBUG
                    std::cout << "[DBUG] URB Delivering: " << id.author << " " << id.seq << "\n";
#endif
                    DeliverInternal(id, true);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(kFinishDeliveringAllMs));
        }
    }