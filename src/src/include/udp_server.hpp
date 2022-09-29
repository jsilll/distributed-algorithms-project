#pragma once

#include <arpa/inet.h>
// #include <netdb.h>
// #include <sys/socket.h>
// #include <sys/types.h>

class UDPserver
{
private:
    int socketDescriptor;
    struct sockaddr_in server_addr;

public:
    UDPserver(in_addr_t ip, unsigned short port);
    virtual ~UDPserver();
    virtual ssize_t receive(char *buffer);
};