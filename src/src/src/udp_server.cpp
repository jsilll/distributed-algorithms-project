#include "udp_server.hpp"

#include <string>
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
#ifdef DEBUG
    std::cout << "[DBUG] Creating new thread: UDPServer::Receive\n";
#endif
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
#ifdef UDP_SEVER_OPTIMIZED
        char buffer[kMaxSendSize];
#else
        auto buffer_unique = std::make_unique<char[]>(max_receive_size_);
        auto buffer = buffer_unique.get();
#endif
        sockaddr_in addr{};
        socklen_t addr_len = sizeof(addr);
        ssize_t len = recvfrom(sockfd_, buffer, max_receive_size_, 0, reinterpret_cast<sockaddr *>(&addr), &addr_len);
        if (len > 0)
        {
#ifdef DEBUG
            std::cout << "[DBUG] UDPServer Received message with size: " << len << std::endl;
#endif
            std::vector<char> payload;
            payload.reserve(addr_len);
            std::copy(buffer, buffer + len, std::back_inserter(payload));
            NotifyAll(payload, addr);
        }
    }
}

void UDPServer::Attach(Observer *obs, sockaddr_in addr) noexcept
{
    observers_.mutex.lock();
    observers_.data[Machine{addr.sin_addr.s_addr, addr.sin_port}].push_back(obs);
    observers_.mutex.unlock();
}

void UDPServer::NotifyAll(const std::vector<char> &bytes, sockaddr_in addr)
{
    observers_.mutex.lock_shared();
    std::vector<Observer *> observers(observers_.data[Machine{addr.sin_addr.s_addr, addr.sin_port}]);
    observers_.mutex.unlock_shared();
    for (const auto &obs : observers)
    {
        obs->Notify(bytes);
    }
}
