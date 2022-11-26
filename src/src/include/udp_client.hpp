#pragma once

#include <string>
#include <vector>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>

class UDPServer;

class UDPClient
{
private:
    int sockfd_;
    bool sock_owner_;

public:
    UDPClient();

    explicit UDPClient(int sockfd, bool sock_owner = false);

    ~UDPClient() noexcept;

    [[nodiscard]] ssize_t Send(const std::vector<char> &bytes, sockaddr_in to_addr) const;

    static sockaddr_in Address(in_addr_t ip, unsigned short port) noexcept;
};