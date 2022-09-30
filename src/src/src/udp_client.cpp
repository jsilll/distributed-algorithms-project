#include "udp_client.hpp"

#include <iostream>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>

#define MAX_LENGTH 512

UDPclient::UDPclient(sockaddr_in target_addr)
    : sockfd_(socket(AF_INET, SOCK_DGRAM, 0)), target_addr_(target_addr)
{
    if (sockfd_ < 0)
    {
        std::runtime_error("Cannot create socket.");
    }
}

UDPclient::~UDPclient()
{
    close(sockfd_);
}

ssize_t UDPclient::Send(const std::string &msg)
{
    ssize_t res = sendto(sockfd_, msg.c_str(), MAX_LENGTH, 0, reinterpret_cast<struct sockaddr *>(&target_addr_), sizeof(target_addr_));

    if (res < 0)
    {
        throw std::runtime_error("Error sending message.");
    }

    return res;
}

ssize_t UDPclient::Send(const std::string &msg, sockaddr_in to_addr)
{
    ssize_t res = sendto(sockfd_, msg.c_str(), MAX_LENGTH, 0, reinterpret_cast<struct sockaddr *>(&to_addr), sizeof(to_addr));

    if (res < 0)
    {
        throw std::runtime_error("Error sending message.");
    }

    return res;
}