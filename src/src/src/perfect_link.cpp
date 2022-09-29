#include "perfect_link.hpp"

#include <algorithm>
#include <list>
#include <optional>
#include <string>
#include <thread>

#include "broadcast.hpp"

#define MAX_LENGTH 512

Perfectlink::Perfectlink(Receiver *receiver, Sender *sender, Broadcast *broadcast)
    : receiver(receiver), sender(sender), broadcast(broadcast), link_active(false)
{
    deliver_thread = std::thread(&Perfectlink::deliverThreaded, this);
    send_thread = std::thread(&Perfectlink::sendThreaded, this);
    ack_thread = std::thread(&Perfectlink::ackThreaded, this);

    if (this->sender != nullptr)
    {
        target_id = this->sender->getTargetId();
    }
    this_process_id = this->receiver->getProcessId();
}

Perfectlink::~Perfectlink()
{
}

void Perfectlink::addMessage(const std::string &msg)
{

    char packet[MAX_LENGTH] = {0};
    int ack = 0;
    sprintf(packet, "%-1d%03d%03d%-s%c", ack, this_process_id, target_id, msg.c_str(), '\0');

    messages_to_send.push_back(packet);
}

void Perfectlink::addMessages(const std::list<std::string> &msgs)
{
    for (auto &msg : msgs)
    {
        this->addMessage(msg);
    }
}

void Perfectlink::addOtherLinks(std::vector<Perfectlink *> other_links)
{
    this->other_links = other_links;
}

void Perfectlink::setLinkActive()
{
    link_active = true;
    if (this->sender != nullptr)
    {
        this->sender->setCanSend(true);
    }
}

void Perfectlink::setLinkInactive()
{
    link_active = false;
    if (this->sender != nullptr)
    {
        this->sender->setCanSend(false);
    }
}

void Perfectlink::sendNewMessage(const std::string &msg)
{

    char packet[MAX_LENGTH] = {0};
    int ack = 0;
    sprintf(packet, "%-1d%03d%03d%-s%c", ack, this_process_id, target_id, msg.c_str(), '\0');

    std::lock_guard<std::mutex> lock(messages_to_send_mutex);
    if (link_active & (this->sender != nullptr))
    {

        this->sender->sendMessage(packet);
    }

    messages_to_send.push_back(packet);
}

void Perfectlink::sendThreaded()
{

    while (true)
    {
        if (link_active)
        {
            if (this->sender != nullptr)
            {

                std::lock_guard<std::mutex> lock(messages_to_send_mutex);

                if (!messages_to_send.empty())
                {

                    for (auto &message_to_send : messages_to_send)
                    {
                        this->sender->sendMessage(message_to_send);

                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
        }
    }
}

void Perfectlink::deliverThreaded()
{

    while (true)
    {
        if (link_active)
        {

            char buffer[MAX_LENGTH];

            if (!(this->receiver->receive(buffer) < 0))
            {

                std::string message_received(buffer);

                std::string ack = message_received.substr(0, 1);
                int msg_process_id = stoi(message_received.substr(1, 3));
                int msg_target_id = stoi(message_received.substr(4, 3));
                std::string payload = message_received.substr(7, message_received.size());
                std::string message_to_deliver = message_received.substr(1, message_received.size());

                if ((this_process_id == msg_target_id) & (ack == "0"))
                { // received a message for this process

                    receiver_mutex.lock();
                    if (!this->receiver->hasDelivered(message_to_deliver))
                    {
                        this->receiver->addMessageDelivered(message_to_deliver);

                        broadcast_mutex.lock();
                        broadcast->deliver(message_received);
                        broadcast_mutex.unlock();
                    }
                    receiver_mutex.unlock();

                    message_received[0] = '1'; // send an ack as long as we receive the same message

                    // If message delivered was send to this process from target of this link, this link sends ack
                    if ((msg_target_id == this_process_id) & (msg_process_id == target_id))
                    {
                        std::lock_guard<std::mutex> lock(acks_to_send_mutex);
                        acks_to_send.push_back(message_received);
                    }
                    else
                    { // send ack from correct link

                        std::lock_guard<std::mutex> lock(link_mutex);
                        Perfectlink *correct_link = other_links[static_cast<unsigned long>(msg_process_id) - 1];

                        if (correct_link != nullptr)
                        {
                            correct_link->addAck(message_received);
                        }
                    }

                    // Received an ack for a message this process sent
                }
                else if ((this_process_id == msg_process_id) & (ack == "1"))
                {

                    message_received[0] = '0';

                    if (msg_target_id == target_id)
                    { // This link sent the message

                        std::lock_guard<std::mutex> lock(messages_to_send_mutex);
                        messages_to_send.remove(message_received);
                    }
                    else
                    {

                        std::lock_guard<std::mutex> lock(link_mutex);
                        Perfectlink *correct_link = other_links[static_cast<unsigned long>(msg_process_id) - 1];

                        if (correct_link != nullptr)
                        {
                            correct_link->removeMessage(message_received);
                        }
                    }
                }
            }

            // std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void Perfectlink::ackThreaded()
{

    while (true)
    {
        if (link_active)
        {
            if (this->sender != nullptr)
            {

                std::lock_guard<std::mutex> lock(acks_to_send_mutex);

                acks_to_send.unique();

                if (!acks_to_send.empty())
                {

                    for (auto &ack_to_send : acks_to_send)
                    {
                        this->sender->sendMessage(ack_to_send);

                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    acks_to_send.clear();
                }
            }
            // std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
}

void Perfectlink::addAck(const std::string &ack)
{

    std::lock_guard<std::mutex> lock(acks_to_send_mutex);
    acks_to_send.push_back(ack);
}

void Perfectlink::removeMessage(const std::string &msg)
{
    std::lock_guard<std::mutex> lock(messages_to_send_mutex);
    messages_to_send.remove(msg);
}

void Perfectlink::addMessageRelay(const std::string &msg)
{
    char string_target_id[4];
    sprintf(string_target_id, "%03d", target_id);

    std::string packet(msg);
    packet[0] = '0';
    packet.replace(4, 3, string_target_id);

    std::lock_guard<std::mutex> lock(messages_to_send_mutex);
    messages_to_send.push_back(packet);
}