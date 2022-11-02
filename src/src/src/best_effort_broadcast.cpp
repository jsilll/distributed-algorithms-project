#include "best_effort_broadcast.hpp"

void BestEffortBroadcast::Send(const std::string &msg) noexcept
{
    PerfectLink::Message::Id id{};

    perfect_links_.mutex.lock_shared();
    for (const auto &[pl_id, pl] : perfect_links_.data)
    {
        id = pl->Send(msg);
    }
    perfect_links_.mutex.unlock_shared();

    if (log_)
    {
        std::stringstream ss;
        ss << "b " << id;
        logger_ << ss.str();
    }
}


void BestEffortBroadcast::Deliever(unsigned long long int sender_id, const PerfectLink::Message &msg) noexcept
{
    if (log_)
    {
        std::stringstream ss;
        ss << "d " << sender_id << " " << msg.id;
        logger_ << ss.str();
    }
}
