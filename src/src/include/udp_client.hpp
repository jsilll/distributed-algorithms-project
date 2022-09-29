#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>

class UDPclient
{
private:
    int socketDescriptor;
    sockaddr_in target_addr;

public:
    UDPclient(sockaddr_in target_addr);
    virtual ~UDPclient();
    ssize_t send(const std::string &msg);
    ssize_t send(const std::string &msg, sockaddr_in to_addr);
};