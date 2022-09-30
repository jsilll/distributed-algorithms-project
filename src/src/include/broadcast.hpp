#pragma once

#include <list>
#include <mutex>
#include <string>
#include <vector>

#include "logger.hpp"
#include "perfect_link.hpp"
#include "receiver.hpp"
#include "threaded_list.hpp"

class Broadcast
{
protected:
    bool active_ = true;
    bool concat_ = false;

    // Logger
    Logger &logger_;

    // Upper Layer
    Broadcast *upper_layer_{};
    std::mutex upper_layer_mutex_;

    // Participants
    Receiver &receiver_;
    std::mutex receiver_mutex_;
    std::vector<PerfectLink *> links_;

    // Messages
    std::list<std::string> messages_to_broadcast_;
    ThreadsafeList<std::string> messages_delivered_;

public:
    Broadcast(Logger &logger, Receiver &receiver);
    virtual ~Broadcast() = default;

    void ConcatMessages();
    virtual void StartBroadcast() = 0;
    virtual void Deliver(const std::string &msg) = 0;
    virtual void BroadCastMessage(const std::string &msg) = 0;

    // Getters
    bool concat() const;

    // Setters
    virtual void active(bool active);
    virtual void upper_layer(Broadcast *upper_layer);
    virtual void links(std::vector<PerfectLink *> links_to_add);
    virtual void add_message_to_broadcast(const std::string &msg);

protected:
    void AddDelieveredMessageLog(const std::string &msg);
    virtual void AddSentMessageLog(const std::string &msg);
};