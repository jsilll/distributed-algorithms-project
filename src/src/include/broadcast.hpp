#pragma once

#include "logger.hpp"
#include "perfect_link.hpp"

class Broadcast : public PerfectLink::Manager
{
public:
    struct Message
    {
        typedef unsigned long int Id;

        Id id;
        unsigned long int author_id;
        unsigned long int sender_id;

        std::string payload;

        inline friend bool operator<(const Message &m1, const Message &m2) noexcept
        {
            if (m1.id != m2.id) return m1.id < m2.id;
            if (m1.author_id != m2.author_id) return m1.author_id < m2.author_id;
            return m1.sender_id < m2.sender_id;
        }
    };

private:
    static constexpr int kMsgPrefixSize = 41;

protected:
    unsigned long long id_;
    std::atomic<Message::Id> n_messages_{1};

public:
    explicit Broadcast(Logger &logger, unsigned long long id) noexcept
        : PerfectLink::Manager::Manager(logger), id_(id) {}

    ~Broadcast() noexcept override = default;

    virtual Message::Id Send(const std::string &msg) = 0;

    std::string FormatMessage(unsigned long int author_id, unsigned long int id, const std::string &payload)
    {
        char buffer[UDPServer::kMaxMsgSize];

        if (std::snprintf(buffer, sizeof(buffer), "BRO AID %010llu ID %010lu PAYLOAD %s", id_, id, payload.c_str()) <= 0)
        {
            throw std::runtime_error("Error during formatting.");
        }

        return std::string(buffer);
    }

    static std::optional<Message> Parse(unsigned long long sender_id, const std::string &msg) noexcept
    {
        // BRO AID 0000000000 ID 0000000000 PAYLOAD
        if (msg.size() > kMsgPrefixSize && msg.substr(0, 3) == "BRO"
        && msg.substr(4, 3) == "AID" && msg.substr(19, 2) == "ID" 
        && msg.substr(33, 7) == "PAYLOAD") 
        {
            try
            {
                unsigned long aid = std::stoul(msg.substr(8, 10));
                unsigned long id = std::stoul(msg.substr(22, 10));
                return {{id, aid, sender_id, msg.substr(kMsgPrefixSize)}};
            }
            catch (const std::exception &e)
            {
                return {};
            }
        }

        return {};
    }
};