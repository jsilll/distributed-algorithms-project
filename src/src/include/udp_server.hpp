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
    friend class UDPClient;

public:
    class Observer
    {
        friend class UDPServer;

    public:
        struct Message
        {
            ssize_t bytes;
            sockaddr_in addr;
            std::string payload;
        };

    private:
        virtual void Deliver(Message msg) = 0;
    };

    struct Machine
    {
        in_addr_t ip;
        in_port_t port;
    };

private:
    int sockfd_;

    sockaddr_in server_addr_;
    std::thread receive_thread_;
    std::atomic<bool> active_{true};
    std::map<Machine, std::vector<Observer *>> observers_;

public:
    UDPServer(in_addr_t ip, in_port_t port);

    ~UDPServer();

    void Receive();

    void Attach(Observer *obs, sockaddr_in addr);

    void Notify(Observer::Message msg);

private:
    int sockfd() const;
};

inline bool operator<(UDPServer::Machine m1, UDPServer::Machine m2)
{
    if (m1.ip != m2.ip)
    {
        return m1.ip < m2.ip;
    }

    return m1.port < m2.port;
}