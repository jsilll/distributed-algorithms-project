#pragma once

#include <cmath>

#include "best_effort_broadcast.hpp"

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
    Shared<std::set<Broadcast::Message::Id>> pending_;
    Shared<std::set<Broadcast::Message::Id>> delivered_;
    Shared<std::map<Message::Id, std::unordered_set<PerfectLink::Id>>> ack_;

    std::thread deliver_thread_;

public:
    explicit UniformReliableBroadcast(Logger &logger, unsigned long long id) noexcept
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
        pending_.mutex.lock();
        pending_.data.insert(msg.id);
        pending_.mutex.unlock();

        BestEffortBroadcast::SendInternal(msg);
    }

    void NotifyInternal(const Broadcast::Message &msg) noexcept override
    {
        BestEffortBroadcast::NotifyInternal(msg);

        ack_.mutex.lock();
        ack_.data[msg.id].insert(msg.sender);
        ack_.mutex.unlock();

        pending_.mutex.lock_shared();
        bool not_pending = pending_.data.count(msg.id) == 0;
        pending_.mutex.unlock_shared();

        delivered_.mutex.lock_shared();
        bool not_delivered = delivered_.data.count(msg.id) == 0;
        delivered_.mutex.unlock_shared();

        if (not_pending && not_delivered)
        {
            pending_.mutex.lock();
            pending_.data.insert(msg.id);
            pending_.mutex.unlock();
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
            pending_.mutex.lock();
            auto pending_messages = std::vector<Broadcast::Message::Id>(pending_.data.begin(), pending_.data.end());
#ifdef DEBUG
            std::cout << "[DBUG] URB Pending: " << pending_.data.size() << "\n";
#endif
            pending_.mutex.unlock();

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
                    pending_.mutex.lock();
                    pending_.data.erase(id);
                    pending_.mutex.unlock();

                    delivered_.mutex.lock();
                    delivered_.data.insert(id);
                    delivered_.mutex.unlock();
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