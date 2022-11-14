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

#define UDP_SERVER_MAX_MSG_SIZE 32

class UDPClient;

struct Machine
{
    in_addr_t ip;
    in_port_t port;

    inline friend bool operator<(Machine m1, Machine m2) noexcept
    {
        if (m1.ip != m2.ip)
        {
            return m1.ip < m2.ip;
        }

        return m1.port < m2.port;
    }

    inline bool friend operator==(const Machine &m1, const Machine &m2)
    {
        return m1.ip == m2.ip && m1.port == m2.port;
    }
};

template <>
struct std::hash<Machine>
{
    inline std::size_t operator()(const Machine  &m) const noexcept
    {
        std::size_t h1 = std::hash<in_addr_t>{}(m.ip);
        std::size_t h2 = std::hash<in_port_t>{}(m.port);
        return h1 ^ (h2 < 1);
    }
};

class UDPServer
{
public:
    class Observer
    {
        friend class UDPServer;

    protected:
        virtual ~Observer() = default;

        virtual void Notify(const std::vector<char> &bytes) = 0;
    };

    static constexpr size_t kMaxSendSize = UDP_SERVER_MAX_MSG_SIZE;

private:
    int sockfd_;
    sockaddr_in server_addr_;
    std::atomic_bool on_{false};

    std::thread receive_thread_;

    Shared<std::unordered_map<Machine, std::vector<Observer *>>> observers_;

public:
    UDPServer(in_addr_t ip, in_port_t port);

    ~UDPServer() noexcept = default;

    void Start() noexcept;

    void Stop() noexcept;

    void Attach(Observer *obs, sockaddr_in addr) noexcept;

    [[nodiscard]] int sockfd() const noexcept;

private:
    void Receive() noexcept;

    void NotifyAll(const std::vector<char> &bytes, sockaddr_in addr);
};
