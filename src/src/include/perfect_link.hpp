#pragma once

#include <set>
#include <map>
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

#include "udp_server.hpp"
#include "udp_client.hpp"
#include "logger.hpp"

class PerfectLink final : public UDPServer::Observer
{
public:
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

    struct Ack
    {
        Message::Id id;

        inline friend bool operator<(Ack ack1, Ack ack2) noexcept
        {
            return ack1.id < ack2.id;
        }
    };

private:
    template <typename T>
    struct Shared
    {
        T data{};
        std::shared_mutex mutex{};
    };

public:
    class Manager
    {
        friend class PerfectLink;

    protected:
        static constexpr int kFinishSendingAllAcksMs = 250;
        static constexpr int kFinishSendingAllMsgsMs = 250;

        std::thread ack_thread_;
        std::thread send_thread_;
        std::atomic_bool on_{false};
        Shared<std::map<unsigned long int, std::unique_ptr<PerfectLink>>> perfect_links_;

    public:
        Manager() noexcept = default;

        virtual ~Manager() noexcept = default;

        void Start() noexcept;
        void Stop() noexcept;

        void Add(std::unique_ptr<PerfectLink> pl) noexcept;

    private:
        void SendAcks();
        void SendMessages();
    };

    class BasicManager final : public Manager
    {
    public:
        void Send(unsigned long long int receiver_id, const std::string &msg) noexcept;
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
    const unsigned long int id_;

    const unsigned long int target_id_;
    const sockaddr_in target_addr_;

    std::atomic<Message::Id> n_messages_{1};

    UDPClient &client_;
    UDPServer &server_;

    Shared<std::set<Ack>> acks_to_send_{};
    Shared<std::set<Message>> messages_to_send_{};
    Shared<std::map<Message::Id, time_t>> messages_delivered_{};

    Logger &logger_;

public:
    PerfectLink() = delete;
    PerfectLink(const PerfectLink &) = delete;
    PerfectLink(PerfectLink &&) = delete;

    PerfectLink(unsigned long int id_,
                unsigned long int target_id_,
                in_addr_t receiver_ip,
                unsigned short receiver_port,
                UDPServer &server,
                UDPClient &client,
                Logger &logger);

    void Send(const std::string &msg) noexcept;

private:
    void SendAcks();
    void CleanAcks() noexcept;
    void SendMessages();

    void Deliver(const std::string &msg) noexcept override;
    static std::optional<std::variant<Message, Ack>> Parse(const std::string &msg) noexcept;

    friend class PerfectLink::Manager;
};