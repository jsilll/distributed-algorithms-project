#pragma once

#include <atomic>
#include <string>
#include <vector>

#include "threaded_list.hpp"
#include "udp_server.hpp"

class Receiver : public UDPserver
{
private:
    int process_id_;
    std::atomic<bool> can_receive_;

    ThreadsafeList<std::string> process_log_;
    ThreadsafeList<std::string> process_delivered_;

public:
    Receiver(in_addr_t ip, unsigned short port, int process_id);

    // ---------- Getters ---------- //

    int process_id() const;
    std::vector<std::string> delivered() const;
    std::vector<std::string> message_log() const;
    bool has_delivered(const std::string &msg) const;

    // ---------- Setters ---------- //

    void can_receive(bool bool_);
    void add_message_log(const std::string &msg);
    void add_messaged_delivered(const std::string &msg);
};