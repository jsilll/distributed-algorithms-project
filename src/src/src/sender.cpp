#include "sender.hpp"

#include <string>

Sender::Sender(sockaddr_in target_addr, int target_id)
    : UDPclient::UDPclient(target_addr), target_id(target_id), msg_to_send(""), can_send(false)
{
}

ssize_t Sender::send()
{
    if (msg_to_send != "")
    {
        return UDPclient::send(msg_to_send.c_str());
    }
    else
    {
        return -1;
    }
}

ssize_t Sender::sendMessage(const std::string &msg)
{
    return UDPclient::send(msg.c_str());
}

int Sender::getTargetId() const
{
    return target_id;
}

void Sender::setMessageToSend(const std::string &msg)
{
    msg_to_send = msg;
}

std::string Sender::getMessageToSend() const
{
    return msg_to_send;
}

void Sender::setCanSend(bool bool_)
{
    can_send = bool_;
}