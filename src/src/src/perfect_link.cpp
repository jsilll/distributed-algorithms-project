#include "perfect_link.hpp"

#include <algorithm>
#include <list>
#include <sstream>
#include <thread>

#ifdef DEBUG
#include <iostream>
#endif

void PerfectLink::Manager::Start() noexcept
{
  on_.store(true);
#ifdef DEBUG
  std::cout << "[DBUG] Creating new thread: PerfectLink::Manager::SendAcks\n";
#endif
  ack_thread_ = std::thread(&PerfectLink::Manager::SendAcks, this);
#ifdef DEBUG
  std::cout << "[DBUG] Creating new thread: PerfectLink::Manager::SendMessages\n";
#endif
  send_thread_ = std::thread(&PerfectLink::Manager::SendMessages, this);
}

void PerfectLink::Manager::Stop() noexcept
{
  if (on_.load())
  {
    on_.store(false);
    ack_thread_.join();
    send_thread_.join();
  }
}

void PerfectLink::Manager::Add(std::unique_ptr<PerfectLink> pl) noexcept
{
  const PerfectLink::Id id = pl->target_id();

  perfect_links_.mutex.lock();
  perfect_links_.data[id] = std::move(pl);
  perfect_links_.data[id]->Subscribe(this);
  perfect_links_.mutex.unlock();

  n_processes_.fetch_add(1);
}

void PerfectLink::Manager::SendAcks()
{
  while (on_.load())
  {
    std::vector<PerfectLink *> pls;

    perfect_links_.mutex.lock_shared();
    pls.reserve(perfect_links_.data.size());
    for (const auto &[_, pl] : perfect_links_.data)
    {
      pls.push_back(pl.get());
    }
    perfect_links_.mutex.unlock_shared();

    for (const auto &pl : pls)
    {
      pl->SendAcks();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(kFinishSendingAllAcksMs));
  }
}

void PerfectLink::Manager::SendMessages()
{
  while (on_.load())
  {
    std::vector<PerfectLink *> pls;

    perfect_links_.mutex.lock_shared();
    pls.reserve(perfect_links_.data.size());
    for (const auto &[_, pl] : perfect_links_.data)
    {
      pls.push_back(pl.get());
    }
    perfect_links_.mutex.unlock_shared();

    for (const auto &pl : pls)
    {
      pl->SendMessages();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(kFinishSendingAllMsgsMs));
  }
}

void PerfectLink::BasicManager::Send(Id receiver_id, const std::string &msg) noexcept
{
  perfect_links_.mutex.lock_shared();
  if (perfect_links_.data.count(receiver_id))
  {
    Message::Seq id = perfect_links_.data[receiver_id]->Send(msg);
    std::stringstream ss;
    ss << "b " << id;
    logger_ << ss.str();
  }
  perfect_links_.mutex.unlock_shared();
}

void PerfectLink::BasicManager::Notify(Id sender_id, const Message &msg) noexcept
{
  std::stringstream ss;
  ss << "d " << sender_id << " " << msg.seq;
  logger_ << ss.str();
}

PerfectLink::PerfectLink(Id id,
                         Id target_id,
                         in_addr_t target_ip,
                         in_port_t target_pot,
                         UDPServer &server,
                         UDPClient &client)
    : id_(id), target_id_(target_id), target_addr_(UDPClient::Address(target_ip, target_pot)), client_(client), server_(server)
{
  server_.Attach(this, target_addr_);
}

PerfectLink::Message::Seq PerfectLink::Send(const std::string &msg) noexcept
{
  Message::Seq id = n_messages_sent_.fetch_add(1);
  messages_to_send_.mutex.lock();
  messages_to_send_.data.insert({id, {msg.begin(), msg.end()}});
  messages_to_send_.mutex.unlock();
  return id;
}

PerfectLink::Message::Seq PerfectLink::Send(const char *payload, std::size_t len) noexcept
{
  Message::Seq id = n_messages_sent_.fetch_add(1);

  messages_to_send_.mutex.lock();
  messages_to_send_.data.insert({id, {payload, payload + len}});
  messages_to_send_.mutex.unlock();

#ifdef DEBUG
  std::cout << "[DBUG] PerfectLink sending Raw Message of size (no metadata): " << len << "\n";
#endif

  return id;
}

void PerfectLink::Subscribe(Manager *manager) noexcept
{
  managers_.emplace_back(manager);
}

void PerfectLink::SendAcks()
{
  acks_to_send_.mutex.lock_shared();
  if (acks_to_send_.data.empty())
  {
    acks_to_send_.mutex.unlock_shared();
    return;
  }

  for (auto ack_id : acks_to_send_.data)
  {
    static_assert(kPacketPrefixSize <= UDPServer::kMaxSendSize);
    char buffer[kPacketPrefixSize];

    PacketType pt{kACK};
    auto pt_ptr = static_cast<char *>(static_cast<void *>(&pt));
    std::copy(pt_ptr, pt_ptr + sizeof(PacketType), buffer);

    auto id_ptr = static_cast<const char *>(static_cast<const void *>(&ack_id));
    std::copy(id_ptr, id_ptr + sizeof(Message::Seq), buffer + sizeof(PacketType));

    try
    {
      [[maybe_unused]] ssize_t bytes = client_.Send(buffer, kPacketPrefixSize, target_addr_);
#ifdef DEBUG
      std::cout << "[DBUG] Sending Ack " << ack_id << " To Process " << target_id_ << "\n";
#endif
    }
    catch (const std::exception &e)
    {
      std::cerr << e.what() << '\n';
    }
  }

  acks_to_send_.mutex.unlock_shared();

  CleanAcks();
}

void PerfectLink::CleanAcks() noexcept
{
  struct AckToRemove
  {
    Message::Seq seq;
    bool remove_from_delivered;
  };

  std::vector<AckToRemove> acks_to_remove;

  messages_delivered_.mutex.lock_shared();
  time_t now = std::time(nullptr);
  for (const auto &msg : messages_delivered_.data)
  {
    auto delta = std::difftime(now, msg.second);
    if (delta >= kStopSendingAcksTimeoutSec)
    {
      acks_to_remove.push_back({msg.first, delta >= kRemoveFromDelivered});
    }
  }

  for (const auto &ack_to_remove : acks_to_remove)
  {
    if (ack_to_remove.remove_from_delivered)
    {
      messages_delivered_.data.erase(ack_to_remove.seq);
    }
  }

  messages_delivered_.mutex.unlock_shared();

  acks_to_send_.mutex.lock();
  for (auto const &ack_to_remove : acks_to_remove)
  {
    acks_to_send_.data.erase({ack_to_remove.seq});
  }
  acks_to_send_.mutex.unlock();
}

void PerfectLink::SendMessages()
{
  messages_to_send_.mutex.lock_shared();

  if (messages_to_send_.data.empty())
  {
    messages_to_send_.mutex.unlock_shared();
    return;
  }

  for (auto &msg : messages_to_send_.data)
  {
    char buffer[UDPServer::kMaxSendSize];
    std::size_t len = Serialize(msg, buffer);

    try
    {
      [[maybe_unused]] ssize_t bytes = client_.Send(buffer, kPacketPrefixSize + msg.payload.size(), target_addr_);
#ifdef DEBUG
      std::cout << "[DBUG] Sending Message " << msg.seq << " To Process " << target_id_ << "\n";
#endif
    }
    catch (const std::exception &e)
    {
      std::cerr << e.what() << '\n';
    }
  }

  messages_to_send_.mutex.unlock_shared();
}

void PerfectLink::Notify(const std::vector<char> &bytes) noexcept
{
  auto parsed_packet = Parse(bytes);

  if (!parsed_packet.has_value())
  {
#ifdef DEBUG
    std::cout << "[DBUG] Invalid Perfect Links Message received.\n";
#endif
    return;
  }

  if (parsed_packet.value().index() == 0)
  {
    // Received a Message
    Message message = std::get<0>(parsed_packet.value());

    acks_to_send_.mutex.lock();
    acks_to_send_.data.insert({message.seq});
    acks_to_send_.mutex.unlock();

    messages_delivered_.mutex.lock();
    bool unseen = messages_delivered_.data.find(message.seq) == messages_delivered_.data.end();
    messages_delivered_.mutex.unlock();

    if (unseen)
    {
      // Its an unseen message
      for (const auto manager : managers_)
      {
        manager->Notify(target_id_, message);
      }
    }

    messages_delivered_.mutex.lock();
    messages_delivered_.data[message.seq] = std::time(nullptr);
    messages_delivered_.mutex.unlock();
  }
  else
  {
    // Received an Ack
    Ack ack_id = std::get<1>(parsed_packet.value());

#ifdef DEBUG
    std::cout << "[DBUG] Received Ack for Message " << ack_id << "\n";
#endif

    messages_to_send_.mutex.lock();
    auto message = messages_to_send_.data.find(Message{ack_id, {}});
    if (message != messages_to_send_.data.end())
    {
#ifdef DEBUG
      std::cout << "[DBUG] Successfully Sent Message " << ack_id << " To Process " << target_id_ << "\n";
#endif
      // If we were sending this message,
      // then stop sending it peer has received
      messages_to_send_.data.erase({ack_id, {}});
    }
    // [else] We've seen this ack before, ignore it
    messages_to_send_.mutex.unlock();
  }
}

std::size_t PerfectLink::Serialize(const Message &msg, char *buffer) noexcept
{
  PacketType pt{kMSG};
  auto pt_ptr = static_cast<char *>(static_cast<void *>(&pt));
  std::copy(pt_ptr, pt_ptr + sizeof(PacketType), buffer);

  auto id_ptr = static_cast<const char *>(static_cast<const void *>(&msg.seq));
  std::copy(id_ptr, id_ptr + sizeof(Message::Seq), buffer + sizeof(PacketType));

  std::copy(msg.payload.begin(), msg.payload.end(), buffer + kPacketPrefixSize);

  return kPacketPrefixSize + msg.payload.size();
}

std::optional<std::variant<PerfectLink::Message, PerfectLink::Ack>> PerfectLink::Parse(const std::vector<char> &bytes) noexcept
{
  if (bytes.size() < kPacketPrefixSize)
  {
    return {};
  }

  if (bytes[0] == kMSG)
  {
    Message::Seq id;
    std::vector<char> payload;
    payload.reserve(bytes.size());
    auto id_ptr = static_cast<char *>(static_cast<void *>(&id));
    std::copy(bytes.begin() + sizeof(PacketType), bytes.begin() + sizeof(PacketType) + sizeof(Message::Seq), id_ptr);
    std::copy(bytes.begin() + kPacketPrefixSize, bytes.end(), std::back_inserter(payload));

#ifdef DEBUG
    std::cout << "[DBUG] PerfectLink received a message with id " << id << " and payload size: " << payload.size() << "\n";
#endif

    return Message{id, payload};
  }
  else if (bytes[0] == kACK)
  {
    Message::Seq id;

    auto id_ptr = static_cast<char *>(static_cast<void *>(&id));
    std::copy(bytes.begin() + sizeof(PacketType), bytes.begin() + sizeof(PacketType) + sizeof(Message::Seq), id_ptr);

    return Ack{id};
  }

  return {};
}
