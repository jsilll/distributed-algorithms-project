#include "udp_client.hpp"

#include <cstring>
#include <unistd.h>
#include <stdexcept>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#ifdef DEBUG
#include <iostream>
#endif

UDPClient::UDPClient()
    : sockfd_(socket(AF_INET, SOCK_DGRAM, 0)), sock_owner_(true)
{
    if (sockfd_ < 0)
    {
        std::runtime_error("Cannot create socket.");
    }
}

UDPClient::UDPClient(int sockfd, bool sock_owner)
    : sockfd_(sockfd), sock_owner_(sock_owner)
{
    if (sockfd_ < 0)
    {
        std::invalid_argument("Invalid socket.");
    }
}

UDPClient::~UDPClient() noexcept
{
    if (sock_owner_)
    {
        close(sockfd_);
    }
}

ssize_t UDPClient::Send(const std::vector<char> &bytes, sockaddr_in to_addr) const
{
#ifdef DEBUG
    std::cout << "[DBUG] UDPClient Sent message of size: " << bytes.size() << std::endl;
#endif

    ssize_t res = sendto(sockfd_, bytes.data(), bytes.size(), 0, reinterpret_cast<struct sockaddr *>(&to_addr), sizeof(to_addr));

    if (res < 0)
    {
        throw std::runtime_error("Error sending message.");
    }

    return res;
}

sockaddr_in UDPClient::Address(in_addr_t ip, in_port_t port) noexcept
{
    sockaddr_in address{};
    address.sin_port = port;
    address.sin_addr.s_addr = ip;
    address.sin_family = AF_INET;
    bzero(address.sin_zero, sizeof(address.sin_zero));
    return address;
}