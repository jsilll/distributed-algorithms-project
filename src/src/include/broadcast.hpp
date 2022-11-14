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

            inline bool friend operator==(const Id &id1, const Id &id2) noexcept
            {
                return id1.author == id2.author && id1.seq == id2.seq;
            }
        };

        Id id;
        PerfectLink::Id sender;
        std::vector<char> payload;
    };

protected:
    static constexpr size_t kPacketPrefixSize = sizeof(PerfectLink::Id) + sizeof(Message::Id::Seq);

protected:
    PerfectLink::Id id_;
    std::atomic<Message::Id::Seq> n_messages_sent_{1};

public:
    explicit Broadcast(Logger &logger, PerfectLink::Id id) noexcept
        : PerfectLink::Manager::Manager(logger), id_(id) {}

    ~Broadcast() noexcept override = default;

    void Send(const std::string &msg) noexcept;

protected:
    void Notify(PerfectLink::Id sender_id, const PerfectLink::Message &msg) noexcept final;

protected:
    virtual void SendInternal(const Broadcast::Message &msg) = 0;

    virtual void NotifyInternal(const Broadcast::Message &msg) = 0;

    virtual void DeliverInternal(const Broadcast::Message::Id &id, bool log) = 0;

private:
    void LogSend(const Broadcast::Message::Id::Seq seq) noexcept;

protected:
    void LogDeliver(const Broadcast::Message::Id &id) noexcept;

public:
    void EncodeMetadata(PerfectLink::Id aid, Broadcast::Message::Id::Seq seq, char *buffer)
    {
        auto aid_ptr = static_cast<char *>(static_cast<void *>(&aid));
        auto seq_ptr = static_cast<char *>(static_cast<void *>(&seq));
        std::copy(aid_ptr, aid_ptr + sizeof(PerfectLink::Id), buffer);
        std::copy(seq_ptr, seq_ptr + sizeof(Broadcast::Message::Id::Seq), buffer + sizeof(PerfectLink::Id));
    }

    static std::optional<Message> Parse(PerfectLink::Id sender_id, const std::vector<char> &bytes) noexcept
    {
        if (bytes.size() <= kPacketPrefixSize)
        {
            std::cout << bytes.size() << " " << kPacketPrefixSize << std::endl;
            return {};
        }

        PerfectLink::Id aid;
        Message::Id::Seq seq;
        std::vector<char> payload;
        payload.reserve(bytes.size());

        auto aid_ptr = static_cast<char *>(static_cast<void *>(&aid));
        auto seq_ptr = static_cast<char *>(static_cast<void *>(&seq));
        std::copy(bytes.begin(), bytes.begin() + sizeof(PerfectLink::Id), aid_ptr);
        std::copy(bytes.begin() + sizeof(PerfectLink::Id), bytes.begin() + kPacketPrefixSize, seq_ptr);
        std::copy(bytes.begin() + kPacketPrefixSize, bytes.end(), std::back_inserter(payload));

        return {{{seq, aid}, sender_id, std::move(payload)}};
    }
};

template <>
struct std::hash<Broadcast::Message::Id>
{
    inline std::size_t operator()(const Broadcast::Message::Id &id) const noexcept
    {
        std::size_t h1 = std::hash<PerfectLink::Id>{}(id.author);
        std::size_t h2 = std::hash<Broadcast::Message::Id::Seq>{}(id.seq);
        return h1 ^ (h2 << 1);
    }
};