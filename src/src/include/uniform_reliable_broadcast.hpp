#pragma once

#include <cmath>
#include <queue>

#include "best_effort_broadcast.hpp"

// Considering the max load on the system will be 2 ** 21
// total broadcast messages, lets try to batch that in 64 batches
#define URB_MAX_MSGS_IN_NETWORK (1 << 15)

/**
 * @brief
 * URB1 (aka RB1) Validity: If a correct process pi broadcasts a message m, then pi eventually delivers m.
 *
 * URB2 (aka RB2) No duplication: No message is delivered more than once.
 *
 * URB3 (aka Rb3) No creation: If a message m is delivered by some process pj, then m was previously broadcasted
 * by some process pi.
 *
 * URB4 Uniform Agreement: If a message m is delivered by some process pi (whether correct or faulty), then m is also
 * eventually delivered by every correct process pj.
 */
class UniformReliableBroadcast : public BestEffortBroadcast
{
private:
    template <typename Id, typename Seq>
    class DeliveredSet
    {
    private:
        struct PeerState
        {
            Seq bottom{1};
            std::set<Seq> delivered; // std::set needed for in order iteration

            bool Contains(Seq seq) const noexcept
            {
                return seq < bottom || delivered.count(seq);
            }

            void Insert(Seq seq) noexcept
            {
                if (seq == bottom)
                {
                    bottom++;

                    std::vector<Seq> to_remove;
                    to_remove.reserve(delivered.size());

                    for (const auto seq : delivered)
                    {
                        if (seq == bottom)
                        {
                            to_remove.emplace_back(seq);
                            bottom++;
                        }
                        else
                        {
                            break;
                        }
                    }

                    for (const auto seq : to_remove)
                    {
                        delivered.erase(seq);
                    }
                }
                else
                {
                    delivered.insert(seq);
                }
            }
        };

    private:
        std::unordered_map<Id, PeerState> state_;

    public:
        bool Contains(const Id id, const Seq seq) const noexcept
        {
            if (state_.count(id))
            {
                return state_.at(id).Contains(seq);
            }

            return false;
        }

        void Insert(const Id id, const Seq seq) noexcept
        {
            state_[id].Insert(seq);
        }

        std::size_t Size() const noexcept
        {
            std::size_t res = 0;
            for (const auto &[_, peer_state] : state_)
            {
                res += peer_state.delivered.size();
            }
            return res;
        }
    };

protected:
    static constexpr int kFinishDeliveringAllMs = 100;

private:
    std::thread deliver_thread_;

protected:
    std::atomic_uint n_own_pending_for_delivery_{0};
    std::atomic_uint n_own_pending_delivery_ideal_{1};
    Shared<std::queue<Broadcast::Message>> own_pending_for_broadcast_;
    Shared<std::unordered_set<Broadcast::Message::Id>> pending_for_delivery_;
    Shared<DeliveredSet<PerfectLink::Id, Broadcast::Message::Id::Seq>> delivered_;
    Shared<std::unordered_map<Message::Id, std::unordered_set<PerfectLink::Id>>> acks_;

public:
    explicit UniformReliableBroadcast(Logger &logger, PerfectLink::Id id) noexcept
        : BestEffortBroadcast(logger, id) {}

    ~UniformReliableBroadcast() noexcept override = default;

    void Stop() noexcept override
    {
        if (on_.load())
        {
            Broadcast::Stop();
            deliver_thread_.join();
        }
    }

    void Start() noexcept override
    {
        Broadcast::Start();
#ifdef DEBUG
        std::cout << "[DBUG] Creating new thread: UniformReliableBroadcast::DeliverPending\n";
#endif
        deliver_thread_ = std::thread(&UniformReliableBroadcast::DeliverPending, this);
    }

    void Add(std::unique_ptr<PerfectLink> pl) noexcept
    {
        const PerfectLink::Id id = pl->target_id();

        perfect_links_.mutex.lock();
        perfect_links_.data[id] = std::move(pl);
        perfect_links_.data[id]->Subscribe(this);
        perfect_links_.mutex.unlock();

        n_processes_.fetch_add(1);
        n_own_pending_delivery_ideal_.store(static_cast<unsigned int>(std::max(1, static_cast<int>(URB_MAX_MSGS_IN_NETWORK /
                                                                                                   std::pow(n_processes_.load(),
                                                                                                            2)))));
    }

    void Send(const std::string &msg) noexcept
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

protected:
    void SendInternal(const Broadcast::Message &msg) noexcept override
    {
        pending_for_delivery_.mutex.lock();
        pending_for_delivery_.data.insert(msg.id);
        pending_for_delivery_.mutex.unlock();
#ifdef DEBUG
        std::cout << "[DBUG] URB: Actually broadcasting message " << msg.id.seq << " now\n";
#endif
        BestEffortBroadcast::SendInternal(msg);
    }

    void NotifyInternal(const Broadcast::Message &msg) noexcept override
    {
        BestEffortBroadcast::NotifyInternal(msg);

        pending_for_delivery_.mutex.lock_shared();
        bool not_pending = pending_for_delivery_.data.count(msg.id) == 0;
        pending_for_delivery_.mutex.unlock_shared();

        delivered_.mutex.lock_shared();
        bool not_delivered = !delivered_.data.Contains(msg.id.author, msg.id.seq);
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

    void DeliverInternal(const Broadcast::Message::Id &id, bool log = false) noexcept override
    {
        if (log)
        {
            LogDeliver(id);
        }
    }

private:
    void DeliverPending() noexcept
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
                bool not_delivered = !delivered_.data.Contains(id.author, id.seq);
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
                    delivered_.data.Insert(id.author, id.seq);
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
};
