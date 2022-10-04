#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <set>
#include <shared_mutex>
#include <variant>
#include <optional>
#include <ctime>
#include <map>

#include "udp_server.hpp"
#include "udp_client.hpp"
#include "logger.hpp"

class PerfectLink final : public UDPServer::Observer
{
public:
    struct Message
    {
        typedef unsigned long int message_id_t;

        message_id_t id;
        std::string payload;

        inline friend bool operator<(const Message &m1, const Message &m2)
        {
            return m1.id < m2.id;
        }
    };

    struct Ack
    {
        Message::message_id_t id;

        inline friend bool operator<(Ack ack1, Ack ack2)
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

private:
    static constexpr int kAckSize = 14;
    static constexpr int kMsgPrefixSize = 23;

    static constexpr int kNoAcksToSendTimeoutMs = 1000;
    static constexpr int kNoMsgsToSendTimeoutMs = 1000;

    static constexpr int kFinishSendingAllAcksMs = 500;
    static constexpr int kFinishSendingAllMsgsMs = 500;

    /**
     * @brief This value should be the result of:
     * kFinishSendingAllMsgsMs + Network Delay + Peer Processing Time
     *
     */
    static constexpr double kStopSendingAcksTimeoutSec = static_cast<double>(kFinishSendingAllMsgsMs + 250 + 100) / 1000.0;

private:
    const unsigned long int id_;

    const unsigned long int target_id_;
    const sockaddr_in target_addr_;

    std::atomic<Message::message_id_t> n_messages_{1};

    UDPClient &client_;
    UDPServer &server_;

    std::thread ack_thread_;
    std::thread send_thread_;

    Shared<std::set<Ack>> acks_to_send_{};
    Shared<std::set<Message>> messages_to_send_{};
    Shared<std::map<Message::message_id_t, time_t>> messages_delivered_{};

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

    ~PerfectLink() final;

    void Send(const std::string &msg);

private:
    void SendAcks();
    void CleanAcks();
    void SendMessages();
    void Deliver(const std::string &msg) override;
    static std::optional<std::variant<Message, Ack>> Parse(const std::string &msg);
};