#include "udp_server.hpp"

#include <iostream>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <unistd.h>
#include <cstring>

#define MAX_LENGTH 512

UDPserver::UDPserver(in_addr_t ip, unsigned short port)
    : sockfd_(socket(AF_INET, SOCK_DGRAM, 0))
{
    if (sockfd_ < 0)
    {
        throw std::runtime_error("Cannot create socket.");
    }

    server_addr_.sin_port = port;
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = ip;
    memset(server_addr_.sin_zero, '\0', sizeof(server_addr_.sin_zero));

    if (bind(sockfd_, reinterpret_cast<const struct sockaddr *>(&server_addr_), sizeof(server_addr_)) < 0)
    {
        throw std::runtime_error("Could bind to socket.");
    }
}

UDPserver::~UDPserver()
{
    close(sockfd_);
}

std::string UDPserver::Receive()
{
    char buffer[MAX_LENGTH];
    socklen_t client_addr_len;
    struct sockaddr_in client_addr;

    ssize_t res_rec = recvfrom(sockfd_, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr *>(&client_addr), &client_addr_len);

    if (res_rec < 0)
    {
        std::runtime_error("Error receiving message.");
    }

    return std::string(buffer);
}