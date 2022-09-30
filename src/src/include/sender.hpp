#pragma once

#include <atomic>
#include <iostream>
#include <string>

#include "udp_client.hpp"

class Sender : public UDPclient
{
private:
    int target_id_;
    std::string msg_to_send_;
    std::atomic<bool> can_send_;

public:
    Sender(sockaddr_in target_addr, int target_id);

    ssize_t Send();
    ssize_t SendMessage(const std::string &msg);

    // ---------- Getters ---------- //

    int target_id() const;
    std::string msg_to_send() const;

    // ---------- Setters ---------- //

    void can_send(bool bool_);
    void msg_to_send(const std::string &msg);
};