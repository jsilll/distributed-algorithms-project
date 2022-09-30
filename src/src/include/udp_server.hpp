#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <string>

class UDPserver
{
private:
    int sockfd_;
    struct sockaddr_in server_addr_;

public:
    UDPserver(in_addr_t ip, unsigned short port);
    virtual ~UDPserver();

    virtual std::string Receive();
};