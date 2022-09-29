#pragma once

#include <iostream>
#include <string>
#include <atomic>

#include "udp_client.hpp"

class Sender : public UDPclient
{
private:
    int target_id;
    std::string msg_to_send;
    std::atomic<bool> can_send;

public:
    Sender(sockaddr_in target_addr, int target_id);
    ssize_t send();
    ssize_t sendMessage(const std::string &msg);
    int getTargetId() const;
    void setMessageToSend(const std::string &msg);
    std::string getMessageToSend() const;
    void setCanSend(bool bool_);
};