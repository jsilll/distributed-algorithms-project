#pragma once

#include <map>
#include <list>
#include <vector>
#include <atomic>
#include <thread>
#include <string>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <shared_mutex>
#include <sys/socket.h>
#include <unordered_map>

#include "shared.hpp"

class UDPClient;

class UDPServer
{
public:
    class Observer
    {
        friend class UDPServer;

    protected:
        virtual ~Observer() = default;

        virtual void Notify(const std::string &msg) = 0;
    };

    struct Machine
    {
        in_addr_t ip;
        in_port_t port;
    };

    static constexpr int kMaxMsgSize = 1024;

private:
    int sockfd_;
    sockaddr_in server_addr_;
    std::atomic_bool on_{false};

    std::thread receive_thread_;

    Shared<std::map<Machine, std::vector<Observer *>>> observers_;

public:
    UDPServer(in_addr_t ip, in_port_t port);

    ~UDPServer() noexcept = default;

    void Start() noexcept;

    void Stop() noexcept;

    void Attach(Observer *obs, sockaddr_in addr) noexcept;

    [[nodiscard]] int sockfd() const noexcept;
private:

    void Receive() noexcept;

    void NotifyAll(const std::string &msg, sockaddr_in addr);
};

inline bool operator<(UDPServer::Machine m1, UDPServer::Machine m2) noexcept
{
    if (m1.ip != m2.ip)
    {
        return m1.ip < m2.ip;
    }

    return m1.port < m2.port;
}