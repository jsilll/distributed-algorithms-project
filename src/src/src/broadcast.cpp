#include "broadcast.hpp"

#include <sstream>

void Broadcast::Send(const std::string &msg) noexcept
{
    Message message = {{n_messages_sent_.fetch_add(1), id_}, id_,  std::vector<char>(msg.begin(), msg.end())};
    SendInternal(message); // Virtual Call
    LogSend(message.id.seq);
}

void Broadcast::Notify(PerfectLink::Id sender_id, const PerfectLink::Message &msg) noexcept
{
    auto message = Parse(sender_id, msg.payload);

    if (message.has_value())
    {
        NotifyInternal(message.value());
    }
    else
    {
#ifdef DEBUG
        std::cout << "Invalid Broadcast Message received." << std::endl;
#endif
    }
}

void Broadcast::LogSend(const Message::Id::Seq seq) noexcept
{
    std::stringstream ss;
    ss << "b " << seq;
    logger_ << ss.str();
}

void Broadcast::LogDeliver(const Message::Id &id) noexcept
{
    std::stringstream ss;
    ss << "d " << id.author << " " << id.seq;
    logger_ << ss.str();
}