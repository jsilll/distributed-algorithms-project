#include "broadcast.hpp"

#include <sstream>

void Broadcast::Send(const std::string &msg) noexcept
{
    Message::Id::Seq seq = n_messages_sent_.fetch_add(1);
    Message message = {{seq, id_}, id_, {msg.begin(), msg.end()}};
    LogSend(seq);
    SendInternal(message);
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

std::size_t Broadcast::Serialize(const Broadcast::Message &msg, char *buffer) noexcept
{

    auto aid_ptr = static_cast<const char *>(static_cast<const void *>(&msg.id.author));
    auto seq_ptr = static_cast<const char *>(static_cast<const void *>(&msg.id.seq));
    std::copy(aid_ptr, aid_ptr + sizeof(PerfectLink::Id), buffer);
    std::copy(seq_ptr, seq_ptr + sizeof(Broadcast::Message::Id::Seq), buffer + sizeof(PerfectLink::Id));
    std::copy(msg.payload.begin(), msg.payload.end(), buffer + kPacketPrefixSize);
    return kPacketPrefixSize + msg.payload.size();
}

  std::optional<Broadcast::Message> Broadcast::Parse(PerfectLink::Id sender_id, const std::vector<char> &bytes) noexcept
  {
    if (bytes.size() < kPacketPrefixSize)
    {
      return {};
    }

    PerfectLink::Id aid;
    Message::Id::Seq seq;

    std::vector<char> payload;
    payload.reserve(bytes.size());

    auto aid_ptr = static_cast<char *>(static_cast<void *>(&aid));
    auto seq_ptr = static_cast<char *>(static_cast<void *>(&seq));
    std::copy(bytes.begin(), bytes.begin() + sizeof(PerfectLink::Id), aid_ptr);
    std::copy(bytes.begin() + sizeof(PerfectLink::Id), bytes.begin() + kPacketPrefixSize, seq_ptr);
    std::copy(bytes.begin() + kPacketPrefixSize, bytes.end(), std::back_inserter(payload));

    return {{{seq, aid}, sender_id, payload}};
  }