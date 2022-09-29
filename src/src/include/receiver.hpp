#pragma once

#include <list>
#include <string>
#include <atomic>

#include "udp_server.hpp"
#include "threaded_list.hpp"

class Receiver : public UDPserver
{
private:
    int process_id;
    std::atomic<bool> can_receive;

    ThreadsafeList process_delivered;
    ThreadsafeList process_log;

public:
    Receiver(in_addr_t ip, unsigned short port, int process_id);
    ssize_t receive(char *buffer) override;
    int getProcessId() const;
    void setCanReceive(bool bool_);
    void addMessageDelivered(const std::string &msg);
    bool hasDelivered(const std::string &msg) const;
    std::list<std::string> getMessagesDelivered() const;
    void addMessageLog(const std::string &msg);
    std::list<std::string> getMessageLog() const;
};