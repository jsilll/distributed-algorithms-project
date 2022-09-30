#include "receiver.hpp"

#include <algorithm>
#include <string>
#include <vector>

Receiver::Receiver(in_addr_t ip, unsigned short port, int process_id)
    : UDPserver::UDPserver(ip, port), process_id_(process_id), can_receive_(true)
{
}

ssize_t Receiver::Receive(char *buffer)
{
    if (can_receive_)
    {
        return UDPserver::Receive(buffer);
    }
    else
    {
        return -1;
    }
}

// ---------- Getters ---------- //

int Receiver::process_id() const
{
    return process_id_;
}

void Receiver::add_message_log(const std::string &msg)
{

    if (!process_log_.Contains(msg))
    {
        process_log_.PushBack(msg);
    }
}

void Receiver::add_messaged_delivered(const std::string &msg)
{
    process_delivered_.PushBack(msg);
}

// ---------- Setters ---------- //

void Receiver::can_receive(bool bool_)
{
    can_receive_ = bool_;
}

std::vector<std::string> Receiver::delivered() const
{

    if (!can_receive_)
    {
        return process_delivered_.vector();
    }
    else
    {
        return std::vector<std::string>();
    }
}

std::vector<std::string> Receiver::message_log() const
{
    if (!can_receive_)
    {
        return process_log_.vector();
    }
    else
    {
        return {};
    }
}

bool Receiver::has_delivered(const std::string &msg) const
{
    return process_delivered_.Contains(msg);
}
