#pragma once

#include <set>
#include <list>
#include <ctime>
#include <mutex>
#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <variant>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

#include "logger.hpp"
#include "udp_server.hpp"
#include "udp_client.hpp"

class PerfectLink final : public UDPServer::Observer
{
public:
    typedef unsigned long int Id;

    enum PacketType : bool
    {
        kACK,
        kMSG,
    };

    struct Message
    {
        typedef unsigned long int Id;

        Id id;
        std::vector<char> payload;

        inline friend bool operator<(const Message &m1, const Message &m2) noexcept
        {
            return m1.id < m2.id;
        }

        inline friend bool operator==(const Message &m1, const Message &m2) noexcept
        {
            return m1.id == m2.id;
        }

        struct Hash
        {
            inline std::size_t operator()(const Message &m) const noexcept
            {
                return std::hash<Id>{}(m.id);
            }
        };
    };

    typedef Message::Id Ack;

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
    static constexpr size_t kPacketPrefixSize = sizeof(PacketType) + sizeof(Message::Id);

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

    std::atomic<Message::Id> n_messages_sent_{1};

    UDPClient &client_;
    UDPServer &server_;

    Shared<std::unordered_set<Ack>> acks_to_send_;
    Shared<std::unordered_set<Message, Message::Hash>> messages_to_send_;
    Shared<std::unordered_map<Message::Id, time_t>> messages_delivered_;

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

    Message::Id Send(const std::string &msg) noexcept;
    Message::Id Send(const std::vector<char> payload) noexcept;

protected:
    friend class PerfectLink::Manager;

    void Subscribe(Manager *manager) noexcept;

private:
    void SendAcks();
    void CleanAcks() noexcept;
    void SendMessages();

    void Notify(const std::vector<char> &bytes) noexcept final;

    static void EncodeMetadata(PacketType type, Message::Id id, char *buffer) noexcept;
    static std::optional<std::variant<Message, Ack>> Parse(const std::vector<char> &bytes) noexcept;
};