#pragma once

#include <cmath>
#include <queue>

#include "best_effort_broadcast.hpp"

// Considering the max load on the system will be 2**21
// total broadcast messages, lets try to batch that in 64 batches
#define URB_MAX_MSGS_IN_NETWORK (1 << 14)

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
protected:
    static constexpr int kFinishDeliveringAllMs = 250;

private:
    std::thread deliver_thread_;

protected:
    std::atomic_uint n_own_pending_for_delivery_{0};
    std::atomic_uint n_own_pending_delivery_ideal_{1};
    Shared<std::queue<Broadcast::Message>> pending_for_broadcast_;
    Shared<std::unordered_set<Broadcast::Message::Id>> delivered_;
    Shared<std::unordered_set<Broadcast::Message::Id>> pending_for_delivery_;
    Shared<std::unordered_map<Message::Id, std::unordered_set<PerfectLink::Id>>> ack_;

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
        n_own_pending_delivery_ideal_.store(std::max(1, static_cast<int>(URB_MAX_MSGS_IN_NETWORK / std::pow(n_processes_.load(), 2))));
    }

protected:
    void SendInternal(const Broadcast::Message &msg) noexcept override
    {
        if (n_own_pending_for_delivery_.load() >= n_own_pending_delivery_ideal_.load())
        {
            pending_for_broadcast_.mutex.lock_shared();
            pending_for_broadcast_.data.push(msg);
            pending_for_broadcast_.mutex.unlock_shared();
            return;
        }

        pending_for_delivery_.mutex.lock();
        pending_for_delivery_.data.insert(msg.id);
        pending_for_delivery_.mutex.unlock();

#ifdef DEBUG
    std::cerr << "[DBUG] URB: Actually broadcasting message " << msg.id.seq << " now\n";
#endif
        n_own_pending_for_delivery_.fetch_add(1);

        BestEffortBroadcast::SendInternal(msg);
    }

    void NotifyInternal(const Broadcast::Message &msg) noexcept override
    {
        BestEffortBroadcast::NotifyInternal(msg);

        ack_.mutex.lock();
        ack_.data[msg.id].insert(msg.sender);
        ack_.mutex.unlock();

        pending_for_delivery_.mutex.lock_shared();
        bool not_pending = pending_for_delivery_.data.count(msg.id) == 0;
        pending_for_delivery_.mutex.unlock_shared();

        delivered_.mutex.lock_shared();
        bool not_delivered = delivered_.data.count(msg.id) == 0;
        delivered_.mutex.unlock_shared();

        if (not_pending && not_delivered)
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

            for (const auto id : pending_messages)
            {
                ack_.mutex.lock();
                bool majority_seen = (ack_.data[id].size() + 1) > static_cast<std::size_t>(std::floor(n_processes_.load() / 2));
                ack_.mutex.unlock();

                delivered_.mutex.lock_shared();
                bool not_delivered = delivered_.data.count(id) == 0;
                delivered_.mutex.unlock_shared();

                if (majority_seen && not_delivered)
                {
                    pending_for_delivery_.mutex.lock();
                    pending_for_delivery_.data.erase(id);
                    pending_for_delivery_.mutex.unlock();

                    if (id.author == id_ && (n_own_pending_for_delivery_.fetch_add(-1) - 1) < n_own_pending_delivery_ideal_.load())
                    {
                        pending_for_broadcast_.mutex.lock();
                        bool pending_for_broadcast_not_empty = !pending_for_broadcast_.data.empty();
                        auto &msg = pending_for_broadcast_.data.front();
                        if (pending_for_broadcast_not_empty)
                        {
                            pending_for_broadcast_.data.pop();
                        }
                        pending_for_broadcast_.mutex.unlock();

                        if (pending_for_broadcast_not_empty)
                        {
                            pending_for_delivery_.mutex.lock();
                            pending_for_delivery_.data.insert(msg.id);
                            pending_for_delivery_.mutex.unlock();

#ifdef DEBUG
    std::cerr << "[DBUG] URB: Actually broadcasting message " << msg.id.seq << " now\n";
#endif
                            n_own_pending_for_delivery_.fetch_add(1);
                            
                            BestEffortBroadcast::SendInternal(msg);
                        }
                    }

                    delivered_.mutex.lock();
                    delivered_.data.insert(id);
                    delivered_.mutex.unlock();

                    ack_.mutex.lock();
                    ack_.data.erase(id);
                    ack_.mutex.unlock();
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