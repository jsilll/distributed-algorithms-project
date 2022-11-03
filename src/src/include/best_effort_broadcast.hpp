#pragma once

#include "broadcast.hpp"

class BestEffortBroadcast : public Broadcast
{
public:
    explicit BestEffortBroadcast(Logger &logger, unsigned long long id) noexcept
        : Broadcast(logger, id) {}

    ~BestEffortBroadcast() noexcept override = default;

    Broadcast::Message::Id Send(const std::string &msg) noexcept override;

protected:
    void Deliever(unsigned long long int sender_id, const PerfectLink::Message &msg) noexcept override;

    /**
     * @brief Used by subclasses 
     * 
     * @param msg 
     */
    virtual void DelieverInternal(const Broadcast::Message &msg) noexcept;
};
