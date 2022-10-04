#include "udp_server.hpp"

#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>
#include <unistd.h>

#include "udp_client.hpp"

UDPServer::UDPServer(in_addr_t ip, in_port_t port)
    : sockfd_(socket(AF_INET, SOCK_DGRAM, 0)),
      server_addr_(UDPClient::Address(ip, port)),
      receive_thread_(std::thread(&UDPServer::Receive, this))
{
    if (sockfd_ < 0)
    {
        throw std::runtime_error("Cannot create socket.");
    }

    if (bind(sockfd_, reinterpret_cast<const sockaddr *>(&server_addr_), sizeof(server_addr_)) < 0)
    {
        throw std::runtime_error("Could not bind to socket.");
    }
}

UDPServer::~UDPServer()
{
    close(sockfd_);
    receive_thread_.detach();
}

[[noreturn]] void UDPServer::Receive()
{
    while (true)
    {
        static char buffer[MAX_MSG_SIZE];

        sockaddr_in addr{};
        socklen_t len = sizeof(addr);
        if (recvfrom(sockfd_, buffer, sizeof(buffer), MSG_WAITALL, reinterpret_cast<sockaddr *>(&addr), &len) < 0)
        {
            throw std::runtime_error("Error receiving message.");
        }

        Notify(std::string(buffer), addr);
    }
}

void UDPServer::Attach(Observer *obs, sockaddr_in addr)
{
    observers_[Machine{addr.sin_addr.s_addr, addr.sin_port}].push_back(obs);
}

void UDPServer::Notify(const std::string& msg, sockaddr_in addr)
{
    for (auto const &obs : observers_[Machine{addr.sin_addr.s_addr, addr.sin_port}])
    {
        obs->Deliver(msg);
    }
}

int UDPServer::sockfd() const
{
    return sockfd_;
}
