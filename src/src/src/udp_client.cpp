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
    : target_addr(target_addr)
{
    // Create socket
    socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0);

    if (socketDescriptor < 0)
    {
        std::cerr << "Cannot create socket" << std::endl;
        exit(0);
    }
}

UDPclient::~UDPclient()
{
    close(socketDescriptor);
}

ssize_t UDPclient::send(const std::string &msg)
{

    ssize_t res_send = sendto(socketDescriptor, msg.c_str(), MAX_LENGTH, 0, reinterpret_cast<struct sockaddr *>(&target_addr), sizeof(target_addr));

    if (res_send < 0)
    {
        std::cerr << "Error sending message " << msg << std::endl;
    }

    return res_send;
}

ssize_t UDPclient::send(const std::string &msg, sockaddr_in to_addr)
{

    ssize_t res_send = sendto(socketDescriptor, msg.c_str(), MAX_LENGTH, 0, reinterpret_cast<struct sockaddr *>(&to_addr), sizeof(to_addr));

    if (res_send < 0)
    {
        std::cerr << "Error sending message " << msg << std::endl;
    }

    return res_send;
}