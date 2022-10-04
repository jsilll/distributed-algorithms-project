#include "perfect_link.hpp"

#include <algorithm>
#include <list>
#include <string>
#include <thread>
#include <sstream>
#include <memory>
#include <stdexcept>

#ifdef DEBUG
#include <iostream>
#endif

PerfectLink::PerfectLink(unsigned long int id,
                         const unsigned long int target_id,
                         in_addr_t target_ip,
                         unsigned short target_pot,
                         UDPServer &server,
                         UDPClient &client,
                         Logger &logger)
    : id_(id), target_id_(target_id), target_addr_(UDPClient::Address(target_ip, target_pot)),
      client_(client), server_(server),
      ack_thread_(std::thread(&PerfectLink::SendAcks, this)),
      send_thread_(std::thread(&PerfectLink::SendMessages, this)),
      logger_(logger)
{
    server_.Attach(this, target_addr_);
}

PerfectLink::~PerfectLink()
{
    ack_thread_.detach();
    send_thread_.detach();
}

void PerfectLink::Send(const std::string &msg)
{
    Message::message_id_t id = n_messages_.fetch_add(1);

    messages_to_send_mutex_.lock();
    messages_to_send_.insert(Message{id, msg});
    messages_to_send_mutex_.unlock();

    // Log that we sent message
    // (Technically we didn't send it still, but we will for sure)
    std::stringstream ss;
    ss << "b " << id;
    logger_ << ss.str();
}

void PerfectLink::SendAcks()
{
    while (true)
    {
        acks_to_send_mutex_.lock_shared();

        if (acks_to_send_.empty())
        {
            acks_to_send_mutex_.unlock_shared();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        for (auto &ack : acks_to_send_)
        {
            static char buffer[kACK_SIZE + 1];
            if (std::snprintf(buffer, sizeof(buffer), "ACK %010lu", ack.id) <= 0)
            {
                // TODO: how to handle this exception?
                acks_to_send_mutex_.unlock_shared();
                throw std::runtime_error("Error during formatting.");
            }

            try
            {
                client_.Send(std::string(buffer), target_addr_);
#ifdef DEBUG
                std::cout << "[DBUG] Sending Ack " << ack.id << " To Process " << target_id_ << "\n";
#endif
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
        }

        acks_to_send_mutex_.unlock_shared();

        CleanAcks();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void PerfectLink::CleanAcks()
{
    static const double timeout = 0.5; // 0.5 seconds timeout to stop sending ack's

    std::vector<Message::message_id_t> acks_to_remove;

    messages_delivered_mutex_.lock_shared();
    time_t now = std::time(nullptr);
    for (auto const &msg : messages_delivered_)
    {
        auto delta = std::difftime(now, msg.second);
        if (delta >= timeout)
        {
            acks_to_remove.push_back(msg.first);
        }
    }
    messages_delivered_mutex_.unlock_shared();

    acks_to_send_mutex_.lock();
    for (auto const &ack_id : acks_to_remove)
    {
        acks_to_send_.erase({ack_id});
    }
    acks_to_send_mutex_.unlock();
}

void PerfectLink::SendMessages()
{
    while (true)
    {
        messages_to_send_mutex_.lock_shared();

        if (messages_to_send_.empty())
        {
            messages_to_send_mutex_.unlock_shared();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        for (auto &msg : messages_to_send_)
        {
            static char buffer[UDPServer::MAX_MSG_SIZE];
            if (std::snprintf(buffer, sizeof(buffer), "MSG %010lu PAYLOAD %s", msg.id, msg.payload.c_str()) <= 0)
            {
                // TODO: how to handle this exception?
                messages_to_send_mutex_.unlock_shared();
                throw std::runtime_error("Error during formatting.");
            }

            try
            {
                client_.Send(std::string(buffer), target_addr_);
#ifdef DEBUG
                std::cout << "[DBUG] Sending Message " << msg.id << " To Process " << target_id_ << ": '" << msg.payload << "'\n";
#endif
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
        }

        messages_to_send_mutex_.unlock_shared();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void PerfectLink::Deliver(const std::string &msg)
{
    auto parsed_msg = Parse(msg);

    if (!parsed_msg.has_value())
    {
#ifdef DEBUG
        std::cerr << "[DBUG] Ignoring Invalid Message: '" << msg << "'\n";
#endif
        return;
    }

    if (parsed_msg.value().index() == 0)
    {
        // Received a Message
        Message message = std::get<0>(parsed_msg.value());

        acks_to_send_mutex_.lock();
        acks_to_send_.insert({message.id});
        acks_to_send_mutex_.unlock();

        messages_delivered_mutex_.lock();
        if (messages_delivered_.find(message.id) == messages_delivered_.end())
        {
            // Log that we received a message
            std::stringstream ss;
            ss << "d " << target_id_ << " " << message.id;
            logger_ << ss.str();
        }
        messages_delivered_[message.id] = std::time(nullptr);
        messages_delivered_mutex_.unlock();
    }
    else
    {
        // Received an Ack
        Ack ack = std::get<1>(parsed_msg.value());

#ifdef DEBUG
        std::cout << "[DBUG] Received Ack for Message " << ack.id << "\n";
#endif

        messages_to_send_mutex_.lock();
        auto message = messages_to_send_.find(Message{ack.id, {}});
        if (message != messages_to_send_.end())
        {
#ifdef DEBUG
            std::cout << "[DBUG] Successfully Sent Message " << ack.id << " To Process " << target_id_ << ": '" << (*message).payload << "'" << std::endl;
#endif
            // If we were sending this message, 
            // then stop sending it peer has received
            messages_to_send_.erase({ack.id, {}});
        } 
        // [else] We've seen this ack before, ignore it
        messages_to_send_mutex_.unlock();
    }
}

std::optional<std::variant<PerfectLink::Message, PerfectLink::Ack>> PerfectLink::Parse(std::string msg)
{
    if (msg.size() > kMIN_VALID_MSG_SIZE && msg.substr(0, 3) == "MSG") // MSG <id> PAYLOAD <payload>
    {
        try
        {
            unsigned long id = std::stoul(msg.substr(5, 13));
            return Message{id, msg.substr(23)};
        }
        catch (const std::exception &e)
        {
            return {};
        }
    }
    else if (msg.size() == kACK_SIZE && msg.substr(0, 3) == "ACK") // ACK <id>
    {
        try
        {
            unsigned long id = std::stoul(msg.substr(5, 13));
            return Ack{id};
        }
        catch (const std::exception &e)
        {
            return {};
        }
    }

    return {};
}