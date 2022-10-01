#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <set>

#include "udp_server.hpp"
#include "udp_client.hpp"
#include "logger.hpp"
#include "threaded_list.hpp"

class Broadcast;
class PerfectLink;

class PerfectLink : public UDPServer::Observer
{
private:
    unsigned long int id_;
    const unsigned long int target_id_;

    sockaddr_in target_addr_;

    UDPClient &client_;
    UDPServer &server_;

    std::thread ack_thread_;
    std::thread send_thread_;
    std::thread deliver_thread_;

    std::mutex acks_to_send_mutex_{};
    std::set<std::string> acks_to_send_{};

    std::mutex messages_to_send_mutex_{};
    std::vector<std::string> messages_to_send_{};

    std::mutex delivered_mutex_{};
    std::vector<std::string> delivered_{};

    Logger &logger_;

    std::atomic<bool> active_{true};

public:
    PerfectLink(unsigned long int id_,
                const unsigned long int target_id_,
                in_addr_t receiver_ip,
                unsigned short receiver_port,
                UDPServer &server,
                UDPClient &client,
                Logger &logger);

    ~PerfectLink();

    void Send(const std::string &msg);

private:
    void Ack();
    void SendAll();
    void Deliver(UDPServer::Observer::Message msg) override;
};