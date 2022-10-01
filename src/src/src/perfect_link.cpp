#include "perfect_link.hpp"

#include <algorithm>
#include <list>
#include <optional>
#include <string>
#include <thread>

PerfectLink::PerfectLink(unsigned long int id,
                         const unsigned long int target_id,
                         in_addr_t target_ip,
                         unsigned short target_pot,
                         UDPServer &server,
                         UDPClient &client,
                         Logger &logger)
    : id_(id), target_id_(target_id), target_addr_(UDPClient::Address(target_ip, target_pot)),
      client_(client), server_(server),
      ack_thread_(std::thread(&PerfectLink::Ack, this)),
      send_thread_(std::thread(&PerfectLink::SendAll, this)),
      logger_(logger)
{
    server_.Attach(this, target_addr_);
}

PerfectLink::~PerfectLink()
{
    active_.store(false);
    ack_thread_.join();
    send_thread_.join();
    deliver_thread_.join();
}

void PerfectLink::Send(const std::string &msg)
{
    std::lock_guard<std::mutex> lock(messages_to_send_mutex_);
    messages_to_send_.push_back(msg);
}

void PerfectLink::Ack()
{
    while (active_.load())
    {
        std::lock_guard<std::mutex> lock(acks_to_send_mutex_);

        if (acks_to_send_.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        for (auto &ack : acks_to_send_)
        {
            client_.Send(ack, target_addr_);
        }
        acks_to_send_.clear();
    }
}
void PerfectLink::SendAll()
{
    while (active_.load())
    {
        std::lock_guard<std::mutex> lock(messages_to_send_mutex_);

        if (messages_to_send_.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        for (auto &msg : messages_to_send_)
        {
            client_.Send(msg, target_addr_);
            // TODO: Logger
            std::cout << "[ LOG] Message Sent To Process " << target_id_ << ": " << msg << std::endl;
        }
        messages_to_send_.clear();
    }
}

void PerfectLink::Deliver(UDPServer::Observer::Message msg)
{
    // TODO: handle acks
    std::cout << "[ LOG] Message Received From Process " << target_id_ << ": " << msg.payload << std::endl;
    // TODO: Logger
}
