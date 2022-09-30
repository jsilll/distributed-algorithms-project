#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

class UDPclient
{
private:
    int sockfd_;
    sockaddr_in target_addr_;

public:
    UDPclient(sockaddr_in target_addr);
    virtual ~UDPclient();

    ssize_t Send(const std::string &msg);
    ssize_t Send(const std::string &msg, sockaddr_in to_addr);
};