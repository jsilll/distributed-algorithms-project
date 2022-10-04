#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

class UDPServer;

class UDPClient
{
private:
    int sockfd_;
    bool sock_owner_{};

public:
    UDPClient();

    explicit UDPClient(UDPServer &server);

    ~UDPClient();

    [[nodiscard]] ssize_t Send(const std::string &msg, sockaddr_in to_addr) const;

    static sockaddr_in Address(in_addr_t ip, unsigned short port);
};