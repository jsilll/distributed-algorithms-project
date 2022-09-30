#include "sender.hpp"

#include <string>

Sender::Sender(sockaddr_in target_addr, int target_id)
    : UDPclient::UDPclient(target_addr), target_id_(target_id), msg_to_send_(""), can_send_(false)
{
}

ssize_t Sender::Send()
{
    if (msg_to_send_ != "")
    {
        return UDPclient::send(msg_to_send_.c_str());
    }
    else
    {
        return -1;
    }
}

ssize_t Sender::SendMessage(const std::string &msg)
{
    return UDPclient::send(msg.c_str());
}

int Sender::target_id() const
{
    return target_id_;
}

void Sender::msg_to_send(const std::string &msg)
{
    msg_to_send_ = msg;
}

std::string Sender::msg_to_send() const
{
    return msg_to_send_;
}

void Sender::can_send(bool bool_)
{
    can_send_ = bool_;
}