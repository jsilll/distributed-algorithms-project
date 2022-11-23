#pragma once

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
    class DeliveredSet
    {
    private:
        class PeerState
        {
        private:
            Broadcast::Message::Id::Seq bottom_{1};
            std::set<Broadcast::Message::Id::Seq> delivered_;

        public:
            inline std::size_t Size() const noexcept
            {
                return delivered_.size();
            }

            inline bool Contains(Broadcast::Message::Id::Seq seq) const noexcept
            {
                return seq < bottom_ || delivered_.count(seq);
            }

            void Insert(Broadcast::Message::Id::Seq seq) noexcept;
        };

    private:
        std::unordered_map<PerfectLink::Id, PeerState> state_;

    public:
        inline bool Contains(const Broadcast::Message::Id &id) const noexcept
        {
            if (state_.count(id.author))
            {
                return state_.at(id.author).Contains(id.seq);
            }

            return false;
        }

        inline void Insert(const Broadcast::Message::Id &id) noexcept
        {
            state_[id.author].Insert(id.seq);
        }

        std::size_t Size() const noexcept;
    };

protected:
    static constexpr int kFinishDeliveringAllMs = 100;

private:
    std::thread deliver_thread_;

protected:
    std::atomic_uint n_own_pending_for_delivery_{0};
    std::atomic_uint n_own_pending_delivery_ideal_{1};

    Shared<DeliveredSet> delivered_;
    Shared<std::queue<Broadcast::Message>> own_pending_for_broadcast_;
    Shared<std::unordered_set<Broadcast::Message::Id>> pending_for_delivery_;
    Shared<std::unordered_map<Message::Id, std::unordered_set<PerfectLink::Id>>> acks_;

public:
    explicit UniformReliableBroadcast(Logger &logger, PerfectLink::Id id) noexcept
        : BestEffortBroadcast(logger, id) {}

    ~UniformReliableBroadcast() noexcept override = default;

    inline void Stop() noexcept override
    {
        if (on_.load())
        {
            Broadcast::Stop();
            deliver_thread_.join();
        }
    }

    inline void Start() noexcept override
    {
        Broadcast::Start();
#ifdef DEBUG
        std::cout << "[DBUG] Creating new thread: UniformReliableBroadcast::DeliverPending\n";
#endif
        deliver_thread_ = std::thread(&UniformReliableBroadcast::DeliverPending, this);
    }

    void Add(std::unique_ptr<PerfectLink> pl) noexcept;

    void Send(const std::string &msg) noexcept;

protected:
    inline void SendInternal(const Broadcast::Message &msg) noexcept override
    {
        pending_for_delivery_.mutex.lock();
        pending_for_delivery_.data.insert(msg.id);
        pending_for_delivery_.mutex.unlock();
#ifdef DEBUG
        std::cout << "[DBUG] URB: Actually broadcasting message " << msg.id.seq << " now\n";
#endif
        BestEffortBroadcast::SendInternal(msg);
    }

    void NotifyInternal(const Broadcast::Message &msg) noexcept override;

    inline void DeliverInternal(const Broadcast::Message::Id &id, bool log = false) noexcept override
    {
        if (log)
        {
            LogDeliver(id);
        }
    }

private:
    void DeliverPending() noexcept;
};
