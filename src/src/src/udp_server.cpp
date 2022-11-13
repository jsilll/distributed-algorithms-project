#include "udp_server.hpp"

#include <string>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "udp_client.hpp"

UDPServer::UDPServer(in_addr_t ip, in_port_t port)
    : sockfd_(socket(AF_INET, SOCK_DGRAM, 0)),
      server_addr_(UDPClient::Address(ip, port))
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

void UDPServer::Start() noexcept
{
    on_.store(true);
    receive_thread_ = std::thread(&UDPServer::Receive, this);
}

void UDPServer::Stop() noexcept
{
    if (on_.load())
    {
        on_.store(false);
        shutdown(sockfd_, SHUT_RDWR);
        receive_thread_.join();
    }
}

void UDPServer::Receive() noexcept
{
    while (on_.load())
    {
        char buffer[kMaxMsgSize];
        sockaddr_in addr{};
        socklen_t len = sizeof(addr);
        ssize_t bytes = recvfrom(sockfd_, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr *>(&addr), &len);
        buffer[bytes] = '\0';
        if (bytes > 0)
        {
            NotifyAll(std::string(buffer), addr);
        }
    }
}

void UDPServer::Attach(Observer *obs, sockaddr_in addr) noexcept
{
    observers_.mutex.lock();
    observers_.data[Machine{addr.sin_addr.s_addr, addr.sin_port}].push_back(obs);
    observers_.mutex.unlock();
}

void UDPServer::NotifyAll(const std::string &msg, sockaddr_in addr)
{
    observers_.mutex.lock_shared();
    std::vector<Observer*> observers(observers_.data[Machine{addr.sin_addr.s_addr, addr.sin_port}]);
    observers_.mutex.unlock_shared();

    for (const auto &obs : observers)
    {
        obs->Notify(msg);
    }
}

int UDPServer::sockfd() const noexcept
{
    return sockfd_;
}
