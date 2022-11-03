#include "best_effort_broadcast.hpp"

Broadcast::Message::Id BestEffortBroadcast::Send(const std::string &msg) noexcept
{
    Broadcast::Message::Id message_id = n_messages_.fetch_add(1);

    std::string message;

    try
    {
        message = Broadcast::FormatMessage(id_, message_id, msg);
    }
    catch (const std::exception &e)
    {
        return message_id;
    }

    perfect_links_.mutex.lock_shared();
    for (const auto &[pl_id, pl] : perfect_links_.data)
    {
        pl->Send(message);
    }
    perfect_links_.mutex.unlock_shared();

    return message_id;
}

void BestEffortBroadcast::Deliever(unsigned long long int sender_id, const PerfectLink::Message &msg) noexcept
{
    auto message = Parse(sender_id, msg.payload);

    if (message.has_value())
    {
        // TODO: Do stuff here
    }
    else
    {
#ifdef DEBUG
        std::cout << "Invalid Broadcast message received" << std::endl;
#endif
    }
}

void BestEffortBroadcast::DelieverInternal(const Broadcast::Message &msg) noexcept
{
    // TODO: do something here
}
