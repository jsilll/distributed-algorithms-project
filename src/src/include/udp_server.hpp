#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <list>
#include <map>
#include <vector>
#include <string>
#include <atomic>

class UDPClient;

class UDPServer
{
public:
    class Observer
    {
        friend class UDPServer;

    protected:
        virtual ~Observer() = default;

        virtual void Deliver(const std::string &msg) = 0;
    };

    struct Machine
    {
        in_addr_t ip;
        in_port_t port;
    };

public:
    static const int MAX_MSG_SIZE = 1024;

private:
    int sockfd_;

    sockaddr_in server_addr_;
    std::thread receive_thread_;
    std::map<Machine, std::vector<Observer *>> observers_;

public:
    UDPServer(in_addr_t ip, in_port_t port);

    ~UDPServer();

    [[noreturn]] void Receive();

    void Attach(Observer *obs, sockaddr_in addr);

    void Notify(const std::string& msg, sockaddr_in addr);

private:
    friend class UDPClient;

    [[nodiscard]] int sockfd() const;
};

inline bool operator<(UDPServer::Machine m1, UDPServer::Machine m2)
{
    if (m1.ip != m2.ip)
    {
        return m1.ip < m2.ip;
    }

    return m1.port < m2.port;
}