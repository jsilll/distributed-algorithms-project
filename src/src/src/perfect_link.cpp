#include "perfect_link.hpp"

#include <algorithm>
#include <list>
#include <optional>
#include <string>
#include <thread>

#include "broadcast.hpp"

#define MAX_LENGTH 512

PerfectLink::PerfectLink(Sender &sender, Receiver &receiver, Broadcast &broadcast)
    : target_id_(sender.target_id()), this_process_id_(receiver.process_id()),
      broadcast_(broadcast), sender_(sender), receiver_(receiver),
      ack_thread_(std::thread(&PerfectLink::Ack, this)),
      send_thread_(std::thread(&PerfectLink::Send, this)),
      deliver_thread_(std::thread(&PerfectLink::Deliver, this))
{
}

void PerfectLink::Ack()
{
    while (true)
    {
        if (link_active_)
        {
            std::lock_guard<std::mutex> lock(acks_to_send_mutex_);
            acks_to_send_.unique();
            if (!acks_to_send_.empty())
            {

                for (auto &ack_to_send : acks_to_send_)
                {
                    this->sender_.SendMessage(ack_to_send);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                acks_to_send_.clear();
            }
        }
    }
}

void PerfectLink::Send()
{
    while (true)
    {
        if (link_active_)
        {
            std::lock_guard<std::mutex> lock(messages_to_send_mutex_);
            if (!messages_to_send_.empty())
            {
                for (auto &message_to_send : messages_to_send_)
                {
                    this->sender_.SendMessage(message_to_send);

                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
        }
    }
}

void PerfectLink::Deliver()
{
    while (true)
    {
        if (link_active_)
        {

            std::string message_received = receiver_.Receive();
            if (message_received.size())
            {
                std::string ack = message_received.substr(0, 1);
                int msg_process_id = stoi(message_received.substr(1, 3));
                int msg_target_id = stoi(message_received.substr(4, 3));
                std::string payload = message_received.substr(7, message_received.size());
                std::string message_to_deliver = message_received.substr(1, message_received.size());

                if ((this_process_id_ == msg_target_id) & (ack == "0"))
                { // received a message for this process
                    receiver_mutex_.lock();
                    if (!receiver_.has_delivered(message_to_deliver))
                    {
                        receiver_.add_messaged_delivered(message_to_deliver);
                        broadcast_mutex_.lock();
                        broadcast_.Deliver(message_received);
                        broadcast_mutex_.unlock();
                    }
                    receiver_mutex_.unlock();

                    message_received[0] = '1'; // send an ack as long as we receive the same message

                    // If message delivered was send to this process from target of this link, this link sends ack
                    if ((msg_target_id == this_process_id_) & (msg_process_id == target_id_))
                    {
                        std::lock_guard<std::mutex> lock(acks_to_send_mutex_);
                        acks_to_send_.push_back(message_received);
                    }
                    else
                    { // send ack from correct link

                        std::lock_guard<std::mutex> lock(link_mutex_);
                        PerfectLink *correct_link = other_links_[static_cast<unsigned long>(msg_process_id) - 1];

                        if (correct_link != nullptr)
                        {
                            correct_link->add_ack(message_received);
                        }
                    }

                    // Received an ack for a message this process sent
                }
                else if ((this_process_id_ == msg_process_id) & (ack == "1"))
                {

                    message_received[0] = '0';

                    if (msg_target_id == target_id_)
                    { // This link sent the message

                        std::lock_guard<std::mutex> lock(messages_to_send_mutex_);
                        messages_to_send_.remove(message_received);
                    }
                    else
                    {

                        std::lock_guard<std::mutex> lock(link_mutex_);
                        PerfectLink *correct_link = other_links_[static_cast<unsigned long>(msg_process_id) - 1];

                        if (correct_link != nullptr)
                        {
                            correct_link->remove_message(message_received);
                        }
                    }
                }
            }
        }
    }
}

void PerfectLink::SendMessage(const std::string &msg)
{
    char packet[MAX_LENGTH] = {0};
    int ack = 0;
    sprintf(packet, "%-1d%03d%03d%-s%c", ack, this_process_id_, target_id_, msg.c_str(), '\0');
    std::lock_guard<std::mutex> lock(messages_to_send_mutex_);
    sender_.SendMessage(packet);
    messages_to_send_.push_back(packet);
}

void PerfectLink::AddMessageRelay(const std::string &msg)
{
    char string_target_id[4];
    sprintf(string_target_id, "%03d", target_id_);

    std::string packet(msg);
    packet[0] = '0';
    packet.replace(4, 3, string_target_id);

    std::lock_guard<std::mutex> lock(messages_to_send_mutex_);
    messages_to_send_.push_back(packet);
}

void PerfectLink::active(bool active)
{
    link_active_ = active;
    sender_.can_send(active);
}

void PerfectLink::add_ack(const std::string &ack)
{
    std::lock_guard<std::mutex> lock(acks_to_send_mutex_);
    acks_to_send_.push_back(ack);
}

void PerfectLink::add_message(const std::string &msg)
{
    int ack = 0;
    char packet[MAX_LENGTH] = {0};
    sprintf(packet, "%-1d%03d%03d%-s%c", ack, this_process_id_, target_id_, msg.c_str(), '\0');
    messages_to_send_.push_back(packet);
}

void PerfectLink::remove_message(const std::string &msg)
{
    std::lock_guard<std::mutex> lock(messages_to_send_mutex_);
    messages_to_send_.remove(msg);
}

void PerfectLink::add_messages(const std::list<std::string> &msgs)
{
    for (auto &msg : msgs)
    {
        this->add_message(msg);
    }
}

void PerfectLink::add_links(std::vector<PerfectLink *> other_links)
{
    this->other_links_ = other_links;
}