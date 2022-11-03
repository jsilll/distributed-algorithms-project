#pragma once

#include <sstream>

#include "uniform_reliable_broadcast.hpp"

class FIFOBroadcast : public UniformReliableBroadcast
{
private:
    bool log_;

public:
    explicit FIFOBroadcast(Logger &logger, unsigned long long id, bool log = false) noexcept
        : UniformReliableBroadcast(logger, id), log_(log) {}

    ~FIFOBroadcast() noexcept override = default;

    Broadcast::Message::Id Send(const std::string &msg) noexcept override
    {
        Broadcast::Message::Id id = UniformReliableBroadcast::Send(msg);

        if (log_)
        {
            LogSend(id);
        }

        // TODO: Do stuff after

        return id;
    }

protected:
    void Deliever(unsigned long long int sender_id, const PerfectLink::Message &msg) noexcept override
    {
        auto message = Broadcast::Parse(sender_id, msg.payload);

        if (message.has_value())
        {
            UniformReliableBroadcast::DelieverInternal(message.value());
        }
        else
        {
#ifdef DEBUG
            std::cout << "Invalid Broadcast message received" << std::endl;
#endif
        }

        if (log_)
        {
            LogDeliever(message.value());
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
        UniformReliableBroadcast::DelieverInternal(msg);
    }

private:
    void LogSend(const Broadcast::Message::Id id) noexcept
    {
        std::stringstream ss;
        ss << "b " << id;
        logger_ << ss.str();
    }

    void LogDeliever(const Broadcast::Message &msg) noexcept
    {
        std::stringstream ss;
        ss << "d " << msg.sender_id << " " << msg.id;
        logger_ << ss.str();
    }
};