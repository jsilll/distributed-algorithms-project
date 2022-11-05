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

    struct Message
    {
        typedef unsigned long int Id;

        Id id;
        std::string payload;

        inline friend bool operator<(const Message &m1, const Message &m2) noexcept
        {
            return m1.id < m2.id;
        }
    };

    typedef Message::Id Ack;

public:
    class Manager
    {
        friend class PerfectLink;

    protected:
        static constexpr int kFinishSendingAllAcksMs = 250;
        static constexpr int kFinishSendingAllMsgsMs = 250;

        std::atomic<Id> n_peers_{0};

        std::thread ack_thread_;
        std::thread send_thread_;
        std::atomic_bool on_{false};
        
        Logger &logger_;
        Shared<std::unordered_map<Id, std::unique_ptr<PerfectLink>>> perfect_links_;

    public:
        explicit Manager(Logger& logger) noexcept : logger_(logger) {};

        virtual ~Manager() noexcept = default;

        void Add(std::unique_ptr<PerfectLink> pl) noexcept;

        virtual void Stop() noexcept;
        virtual void Start() noexcept;

    protected:
        void SendAcks();
        void SendMessages();

        virtual void Notify(unsigned long long int sender_id, const Message &msg) = 0;
    };

    class BasicManager final : public Manager
    {
    public:
        explicit BasicManager(Logger &logger) : Manager::Manager(logger) {}

        void Send(unsigned long long int receiver_id, const std::string &msg) noexcept;

    protected:
        void Notify(unsigned long long int sender_id, const Message &msg) noexcept override;
    };

private:
    static constexpr int kAckSize = 14;
    static constexpr int kMsgPrefixSize = 23;

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
    Shared<std::set<Message>> messages_to_send_;
    Shared<std::unordered_map<Message::Id, time_t>> messages_delivered_;

    std::vector<Manager *> managers_;

public:
    PerfectLink() = delete;
    PerfectLink(const PerfectLink &) = delete;
    PerfectLink(PerfectLink &&) = delete;

    PerfectLink(unsigned long int id_,
                unsigned long int target_id_,
                in_addr_t receiver_ip,
                unsigned short receiver_port,
                UDPServer &server,
                UDPClient &client);

    Message::Id Send(const std::string &msg) noexcept;

protected:
    friend class PerfectLink::Manager;

    void Subscribe(Manager* manager) noexcept;

private:
    void SendAcks();
    void CleanAcks() noexcept;
    void SendMessages();

    void Notify(const std::string &msg) noexcept override;
    static std::optional<std::variant<Message, Ack>> Parse(const std::string &msg) noexcept;
};