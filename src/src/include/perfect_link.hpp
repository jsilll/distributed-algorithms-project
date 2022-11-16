#pragma once

#include <atomic>
#include <ctime>
#include <list>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "logger.hpp"
#include "udp_client.hpp"
#include "udp_server.hpp"

class PerfectLink final : public UDPServer::Observer
{
public:
  typedef unsigned int Id;

  enum PacketType : bool
  {
    kACK = false,
    kMSG = true,
  };

  struct Message
  {
    typedef unsigned int Seq;

    Seq seq;
    std::vector<char> payload;

    inline friend bool operator<(const Message &m1, const Message &m2) noexcept
    {
      return m1.seq < m2.seq;
    }

    inline friend bool operator==(const Message &m1, const Message &m2) noexcept
    {
      return m1.seq == m2.seq;
    }

    struct Hash
    {
      inline std::size_t operator()(const Message &m) const noexcept
      {
        return std::hash<Seq>{}(m.seq);
      }
    };
  };

  typedef Message::Seq Ack;

  static constexpr size_t kPacketPrefixSize = sizeof(PacketType) + sizeof(Message::Seq);

public:
  class Manager
  {
    friend class PerfectLink;

  protected:
    static constexpr int kFinishSendingAllAcksMs = 250;
    static constexpr int kFinishSendingAllMsgsMs = 250;

    std::atomic<Id> n_processes_{1};

    std::thread ack_thread_;
    std::thread send_thread_;
    std::atomic_bool on_{false};

    Logger &logger_;
    Shared<std::unordered_map<Id, std::unique_ptr<PerfectLink>>> perfect_links_;

  public:
    explicit Manager(Logger &logger) noexcept : logger_(logger){};

    virtual ~Manager() noexcept = default;

    void Add(std::unique_ptr<PerfectLink> pl) noexcept;

    virtual void Stop() noexcept;
    virtual void Start() noexcept;

  protected:
    void SendAcks();
    void SendMessages();

    virtual void Notify(Id sender_id, const Message &msg) = 0;
  };

  class BasicManager final : public Manager
  {
  public:
    explicit BasicManager(Logger &logger) : Manager::Manager(logger) {}

    void Send(PerfectLink::Id receiver_id, const std::string &msg) noexcept;

  protected:
    void Notify(Id sender_id, const Message &msg) noexcept override;
  };

private:
  /**
   * @brief This value should be the result of:
   * (kFinishSendingAllMsgsMs + Network Delay + Peer Processing Time)
   *
   */
  static constexpr double kStopSendingAcksTimeoutSec = static_cast<double>(Manager::kFinishSendingAllMsgsMs + 250 + 100) / 1000.0;

private:
  const Id id_;
  const Id target_id_;
  const sockaddr_in target_addr_;

  std::atomic<Message::Seq> n_messages_sent_{1};

  UDPClient &client_;
  UDPServer &server_;

  Shared<std::unordered_set<Ack>> acks_to_send_;
  Shared<std::unordered_set<Message, Message::Hash>> messages_to_send_;
  Shared<std::unordered_map<Message::Seq, time_t>> messages_delivered_;

  std::vector<Manager *> managers_;

public:
  PerfectLink() = delete;
  PerfectLink(const PerfectLink &) = delete;
  PerfectLink(PerfectLink &&) = delete;

  PerfectLink(Id id_,
              Id target_id_,
              in_addr_t receiver_ip,
              in_port_t receiver_port,
              UDPServer &server,
              UDPClient &client);

  Message::Seq Send(const std::string &msg) noexcept;
  Message::Seq Send(const char *payload, std::size_t len) noexcept;

protected:
  friend class PerfectLink::Manager;

  void Subscribe(Manager *manager) noexcept;

private:
  void SendAcks();
  void CleanAcks() noexcept;
  void SendMessages();

  void Notify(const std::vector<char> &bytes) noexcept final;

  static std::size_t Serialize(const Message &msg, char *buffer) noexcept;
  static std::optional<std::variant<Message, Ack>> Parse(const std::vector<char> &bytes) noexcept;
};