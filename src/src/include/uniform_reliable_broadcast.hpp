#pragma once

#include <cmath>
#include <queue>

#include "best_effort_broadcast.hpp"

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

protected:
    Shared<std::queue<Broadcast::Message>> pending_for_broadcast_;
    Shared<std::unordered_set<Broadcast::Message::Id>> delivered_;
    Shared<std::unordered_set<Broadcast::Message::Id>> pending_for_delivery_;
    Shared<std::unordered_map<Message::Id, std::unordered_set<PerfectLink::Id>>> ack_;

    std::thread deliver_thread_;

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
        deliver_thread_ = std::thread(&UniformReliableBroadcast::DeliverPending, this);
    }

protected:
    void SendInternal(const Broadcast::Message &msg) noexcept override
    {
        pending_for_delivery_.mutex.lock_shared();
        std::size_t n_pending_for_delivery = pending_for_delivery_.data.size();
        pending_for_delivery_.mutex.unlock_shared();

        std::size_t n_ideal_pending_for_delivery = std::max(1, static_cast<int>(URB_MAX_MSGS_IN_NETWORK / std::pow(n_processes_.load(), 2)));
        if (n_pending_for_delivery > n_ideal_pending_for_delivery)
        {
            pending_for_broadcast_.mutex.lock_shared();
            pending_for_broadcast_.data.push(msg);
            pending_for_broadcast_.mutex.unlock_shared();
            return;
        }

        pending_for_delivery_.mutex.lock();
        pending_for_delivery_.data.insert(msg.id);
        pending_for_delivery_.mutex.unlock();

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
            auto pending_messages = std::vector<Broadcast::Message::Id>(pending_for_delivery_.data.begin(), pending_for_delivery_.data.end());
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
                    pending_for_broadcast_.mutex.lock_shared();
                    auto &msg = pending_for_broadcast_.data.front();
                    pending_for_broadcast_.mutex.unlock_shared();

                    pending_for_delivery_.mutex.lock();
                    pending_for_delivery_.data.erase(id);
                    std::size_t n_pending_for_delivery = pending_for_delivery_.data.size();
                    pending_for_delivery_.mutex.unlock();

                    std::size_t n_ideal_pending_for_delivery = std::max(1, static_cast<int>(URB_MAX_MSGS_IN_NETWORK / std::pow(n_processes_.load(), 2)));
                    if (n_pending_for_delivery < n_ideal_pending_for_delivery)
                    {
                        pending_for_delivery_.mutex.lock();
                        pending_for_delivery_.data.insert(msg.id);
                        pending_for_delivery_.mutex.unlock();

                        pending_for_broadcast_.mutex.lock();
                        pending_for_broadcast_.data.pop();
                        pending_for_broadcast_.mutex.unlock();

                        BestEffortBroadcast::SendInternal(msg);
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
                    DeliverInternal(id, true); // Call the most specified Deliver Internal Possible
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(kFinishDeliveringAllMs));
        }
    }
};