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

class Broadcast;
class PerfectLink;

class PerfectLink : public UDPServer::Observer
{
public:
    struct Message
    {
        typedef unsigned long int message_id_t;
        message_id_t id;
        std::string payload;
    };

    struct Ack
    {
        unsigned long int id;
    };

private:
    static const int kACK_SIZE = 14;
    static const int kMIN_VALID_MSG_SIZE = 23;

private:
    const unsigned long int id_;

    const unsigned long int target_id_;
    sockaddr_in target_addr_;

    std::atomic<Message::message_id_t> n_messages_{0};

    UDPClient &client_;
    UDPServer &server_;

    std::thread ack_thread_;
    std::thread send_thread_;

    std::set<Message> messages_to_send_{};
    std::shared_mutex messages_to_send_mutex_{};

    std::set<Ack> acks_to_send_{};
    std::shared_mutex acks_to_send_mutex_{};

    std::map<Message::message_id_t, time_t> messages_delivered_{};
    std::shared_mutex messages_delivered_mutex_{};

    Logger &logger_;

    std::atomic<bool> stop_{false};
    std::atomic<bool> debug_{false};

public:
    PerfectLink(unsigned long int id_, const unsigned long int target_id_, in_addr_t receiver_ip, unsigned short receiver_port, UDPServer &server, UDPClient &client, Logger &logger);

    ~PerfectLink();

    void Send(const std::string &msg);

    void debug(bool debug);

private:
    void SendAcks();
    void CleanAcks();
    void SendMessages();

    void Deliver(const std::string &msg) override;

    std::optional<std::variant<Message, Ack>> Parse(std::string msg);
};

inline bool operator<(PerfectLink::Message m1, PerfectLink::Message m2)
{
    return m1.id < m2.id;
}

inline bool operator<(PerfectLink::Ack ack1, PerfectLink::Ack ack2)
{
    return ack1.id < ack2.id;
}