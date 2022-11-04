#pragma once

#include "logger.hpp"
#include "perfect_link.hpp"

class Broadcast : public PerfectLink::Manager
{
public:
    struct Message
    {
        struct Id
        {
            typedef unsigned long int Seq;

            Seq seq;
            PerfectLink::Id author;

            inline bool friend operator<(const Id &id1, const Id &id2) noexcept 
            {
                if (id1.seq != id2.seq)
                {
                    return id1.seq < id2.seq;
                }
                
                return id1.author < id2.author;
            }
        };

        Id id;
        PerfectLink::Id sender;

        std::string payload;
    };

private:
    static constexpr int kMsgPrefixSize = 42;

protected:
    PerfectLink::Id id_;
    std::atomic<Message::Id::Seq> n_messages_sent_{1};

public:
    explicit Broadcast(Logger &logger, unsigned long long id) noexcept
        : PerfectLink::Manager::Manager(logger), id_(id) {}

    ~Broadcast() noexcept override = default;

    void Send(const std::string &msg) noexcept;

protected:
    void Notify(unsigned long long int sender_id, const PerfectLink::Message &msg) noexcept final;

protected:
    virtual void SendInternal(const Broadcast::Message &msg) = 0;

    virtual void NotifyInternal(const Broadcast::Message &msg) = 0;

    virtual void DeliverInternal(const Broadcast::Message::Id &id, bool log) = 0; 

private:
    void LogSend(const Broadcast::Message::Id::Seq seq) noexcept;

protected:
    void LogDeliver(const Broadcast::Message::Id &id) noexcept;

public:
    static std::string Format(const Message &msg)
    {
        char buffer[UDPServer::kMaxMsgSize];

        if (std::snprintf(buffer, sizeof(buffer), "BRO AID %010lu SEQ %010lu PAYLOAD %s", msg.id.author, msg.id.seq, msg.payload.c_str()) <= 0)
        {
            throw std::runtime_error("Error during formatting.");
        }

        return std::string(buffer);
    }

    static std::optional<Message> Parse(unsigned long long sender_id, const std::string &msg) noexcept
    {
        // BRO AID 0000000000 ID 0000000000 PAYLOAD
        if (msg.size() > kMsgPrefixSize && msg.substr(0, 3) == "BRO" && msg.substr(4, 3) == "AID" &&
            msg.substr(19, 3) == "SEQ" && msg.substr(34, 7) == "PAYLOAD")
        {
            try
            {
                unsigned long aid = std::stoul(msg.substr(8, 10));
                unsigned long seq = std::stoul(msg.substr(23, 10));
                return {{{seq, aid}, sender_id, msg.substr(kMsgPrefixSize)}};
            }
            catch (const std::exception &e)
            {
                return {};
            }
        }

        return {};
    }
};