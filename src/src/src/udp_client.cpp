#include "udp_client.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "udp_server.hpp"

UDPClient::UDPClient(void)
    : sockfd_(socket(AF_INET, SOCK_DGRAM, 0)), sock_owner_(true)
{
    if (sockfd_ < 0)
    {
        std::runtime_error("Cannot create socket.");
    }
}

UDPClient::UDPClient(UDPServer &server)
    : sockfd_(server.sockfd())
{
    if (sockfd_ < 0)
    {
        std::invalid_argument("Invalid socket.");
    }
}

UDPClient::~UDPClient()
{
    if (sock_owner_)
    {
        close(sockfd_);
    }
}

ssize_t UDPClient::Send(const std::string &msg, sockaddr_in to_addr)
{
    ssize_t res = sendto(sockfd_, msg.c_str(), msg.size(), 0, reinterpret_cast<struct sockaddr *>(&to_addr), sizeof(to_addr));

    if (res < 0)
    {
        throw std::runtime_error("Error sending message.");
    }

    return res;
}

sockaddr_in UDPClient::Address(in_addr_t ip, in_port_t port)
{
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = ip;
    address.sin_port = port;
    bzero(address.sin_zero, sizeof(address.sin_zero));
    return address;
}