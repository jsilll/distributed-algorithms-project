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

    messages_to_send_.mutex.lock();
    messages_to_send_.data.insert(Message{id, msg});
    messages_to_send_.mutex.unlock();

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
        acks_to_send_.mutex.lock_shared();
        if (acks_to_send_.data.empty())
        {
            acks_to_send_.mutex.unlock_shared();
            std::this_thread::sleep_for(std::chrono::milliseconds(kNoAcksToSendTimeoutMs));
            continue;
        }

        for (auto &ack : acks_to_send_.data)
        {
            static char buffer[kAckSize + 1];
            if (std::snprintf(buffer, sizeof(buffer), "ACK %010lu", ack.id) <= 0)
            {
                // TODO: how to handle this exception?
                acks_to_send_.mutex.unlock_shared();
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

        acks_to_send_.mutex.unlock_shared();

        CleanAcks();

        std::this_thread::sleep_for(std::chrono::milliseconds(kFinishSendingAllAcksMs));
    }
}

void PerfectLink::CleanAcks()
{
    std::vector<Message::message_id_t> acks_to_remove;

    messages_delivered_.mutex.lock_shared();
    time_t now = std::time(nullptr);
    for (auto const &msg : messages_delivered_.data)
    {
        auto delta = std::difftime(now, msg.second);
        if (delta >= kStopSendingAcksTimeoutSec)
        {
            acks_to_remove.push_back(msg.first);
        }
    }
    messages_delivered_.mutex.unlock_shared();

    acks_to_send_.mutex.lock();
    for (auto const &ack_id : acks_to_remove)
    {
        acks_to_send_.data.erase({ack_id});
    }
    acks_to_send_.mutex.unlock();
}

void PerfectLink::SendMessages()
{
    while (true)
    {
        messages_to_send_.mutex.lock_shared();

        if (messages_to_send_.data.empty())
        {
            messages_to_send_.mutex.unlock_shared();
            std::this_thread::sleep_for(std::chrono::milliseconds(kNoMsgsToSendTimeoutMs));
            continue;
        }

        for (auto &msg : messages_to_send_.data)
        {
            static char buffer[UDPServer::MAX_MSG_SIZE];
            if (std::snprintf(buffer, sizeof(buffer), "MSG %010lu PAYLOAD %s", msg.id, msg.payload.c_str()) <= 0)
            {
                // TODO: how to handle this exception?
                messages_to_send_.mutex.unlock_shared();
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

        messages_to_send_.mutex.unlock_shared();

        std::this_thread::sleep_for(std::chrono::milliseconds(kFinishSendingAllMsgsMs));
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

        acks_to_send_.mutex.lock();
        acks_to_send_.data.insert({message.id});
        acks_to_send_.mutex.unlock();

        messages_delivered_.mutex.lock();
        if (messages_delivered_.data.find(message.id) == messages_delivered_.data.end())
        {
            // Log that we received a message
            std::stringstream ss;
            ss << "d " << target_id_ << " " << message.id;
            logger_ << ss.str();
        }
        messages_delivered_.data[message.id] = std::time(nullptr);
        messages_delivered_.mutex.unlock();
    }
    else
    {
        // Received an Ack
        Ack ack = std::get<1>(parsed_msg.value());

#ifdef DEBUG
        std::cout << "[DBUG] Received Ack for Message " << ack.id << "\n";
#endif

        messages_to_send_.mutex.lock();
        auto message = messages_to_send_.data.find(Message{ack.id, {}});
        if (message != messages_to_send_.data.end())
        {
#ifdef DEBUG
            std::cout << "[DBUG] Successfully Sent Message " << ack.id << " To Process " << target_id_ << ": '" << (*message).payload << "'" << std::endl;
#endif
            // If we were sending this message, 
            // then stop sending it peer has received
            messages_to_send_.data.erase({ack.id, {}});
        } 
        // [else] We've seen this ack before, ignore it
        messages_to_send_.mutex.unlock();
    }
}

std::optional<std::variant<PerfectLink::Message, PerfectLink::Ack>> PerfectLink::Parse(std::string msg)
{
    if (msg.size() > kMsgPrefixSize && msg.substr(0, 3) == "MSG") // MSG <id> PAYLOAD <payload>
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
    else if (msg.size() == kAckSize && msg.substr(0, 3) == "ACK") // ACK <id>
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