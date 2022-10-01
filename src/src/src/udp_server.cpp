#include "udp_server.hpp"

#include <iostream>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <unistd.h>
#include <cstring>

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

    if (bind(sockfd_, reinterpret_cast<const struct sockaddr *>(&server_addr_), sizeof(server_addr_)) < 0)
    {
        std::cout << " Could bind to socket" << std::endl;
    }
}

UDPServer::~UDPServer()
{
    close(sockfd_);
    active_.store(false);
    receive_thread_.join();
}

void UDPServer::Receive()
{
    while (active_.load())
    {
        static const int MAX_RECV_LEN = 1024;
        static char buffer[MAX_RECV_LEN];

        sockaddr_in from;
        socklen_t len = sizeof(from);
        ssize_t bytes = recvfrom(sockfd_, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr *>(&from), &len);

        if (bytes < 0)
        {
            std::runtime_error("Error receiving message.");
        }

        Notify(Observer::Message{bytes, from, std::string(buffer)});
    }
}

void UDPServer::Attach(Observer *obs, sockaddr_in addr)
{
    observers_[Machine{addr.sin_addr.s_addr, addr.sin_port}].push_back(obs);
}

void UDPServer::Notify(Observer::Message msg)
{
    for (auto const &obs : observers_[Machine{msg.addr.sin_addr.s_addr, msg.addr.sin_port}])
    {
        obs->Deliver(msg);
    }
}

int UDPServer::sockfd() const
{
    return sockfd_;
}
