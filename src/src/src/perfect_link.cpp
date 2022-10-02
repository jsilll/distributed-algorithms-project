#include "perfect_link.hpp"

#include <algorithm>
#include <list>
#include <string>
#include <thread>

#include <memory>
#include <stdexcept>

PerfectLink::PerfectLink(unsigned long int id, const unsigned long int target_id, in_addr_t target_ip, unsigned short target_pot, UDPServer &server, UDPClient &client, Logger &logger)
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
    stop_.store(true);
    ack_thread_.join();
    send_thread_.join();
}

void PerfectLink::Send(const std::string &msg)
{
    messages_to_send_mutex_.lock();
    messages_to_send_.insert(Message{n_messages_.fetch_add(1), msg});
    messages_to_send_mutex_.unlock();
}

void PerfectLink::SendAcks()
{
    while (!stop_.load())
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
                throw std::runtime_error("Error during formatting.");
            }

            try
            {
                client_.Send(std::string(buffer), target_addr_);
                std::cout << "[INFO] Sending Ack " << ack.id << " To Process " << target_id_ << "\n";
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
        }

        acks_to_send_mutex_.unlock_shared();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
void PerfectLink::SendMessages()
{
    while (!stop_.load())
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
                throw std::runtime_error("Error during formatting.");
            }

            try
            {
                client_.Send(std::string(buffer), target_addr_);
                std::cout << "[INFO] Sending Message " << msg.id << " To Process " << target_id_ << ": '" << msg.payload << "'\n";
                // TODO: logger << ...
                // DOUBT: should we only log once we know the message has been delivered
            }
            catch (const std::exception &e)
            {
                // somehow not remove this message (change std::vector -> std::set)
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
        std::cerr << "[INFO] Received Invalid Message: '" << msg << "'\n";
        return;
    }

    if (parsed_msg.value().index() == 0) // Received a Message
    {
        Message message = std::get<0>(parsed_msg.value());

        if (messages_delivered_.find(message) == messages_delivered_.end())
        {
            messages_delivered_.insert(message); // TODO: maybe we should only keep the id's, not the whole message
            std::cout << "[ LOG] Received Message: '" << message.payload << "'\n";
        }

        // TODO: (not here but related)
        // Implement some timeout mechanism for removing acks when no longer receiving the message from peer
        acks_to_send_.insert(Ack{message.id});
    }
    else // Received an Ack
    {
        Ack ack = std::get<1>(parsed_msg.value());
        std::cout << "[INFO] Received Ack: '" << ack.id << "'\n";

        messages_to_send_mutex_.lock();

        auto message = messages_to_send_.find(Message{ack.id, ""});
        if (message != messages_to_send_.end())
        {
            std::cout << "[ LOG] Successfully Sent Message " << ack.id << " To Process " << target_id_ << ": '" << (*message).payload << "'" << std::endl;
            messages_to_send_.erase(Message{ack.id, ""});
        }

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