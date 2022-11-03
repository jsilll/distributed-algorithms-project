#pragma once

#include "best_effort_broadcast.hpp"

class UniformReliableBroadcast : public BestEffortBroadcast
{
public:
    explicit UniformReliableBroadcast(Logger &logger, unsigned long long id) noexcept
        : BestEffortBroadcast(logger, id) {}

    ~UniformReliableBroadcast() noexcept override = default;

    Broadcast::Message::Id Send(const std::string &msg) noexcept override
    {
        // TODO: Do stuff before
        return BestEffortBroadcast::Send(msg);
    }

protected:
    void Deliever(unsigned long long int sender_id, const PerfectLink::Message &msg) noexcept override
    {
        auto message = Broadcast::Parse(sender_id, msg.payload);

        if (message.has_value())
        {
            BestEffortBroadcast::DelieverInternal(message.value());
        }
        else
        {
#ifdef DEBUG
            std::cout << "Invalid Broadcast message received" << std::endl;
#endif
        }

        // TODO: Do stuff after
    }

    /**
     * @brief Used by subclasses
     *
     * @param msg
     */
    virtual void DelieverInternal(const Broadcast::Message &msg) noexcept
    {
        BestEffortBroadcast::DelieverInternal(msg);
    }
};