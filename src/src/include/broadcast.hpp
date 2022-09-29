#pragma once

#include <list>
#include <mutex>
#include <string>
#include <vector>

#include "perfect_link.hpp"
#include "receiver.hpp"
#include "threaded_list.hpp"

class Broadcast
{
protected:
    virtual void addSentMessageLog(const std::string &msg);

    void addDeliveredMessageLog(const std::string &msg);

    Receiver *receiver;
    std::vector<Perfectlink *> links;

    Broadcast *upper_layer;

    std::mutex upper_layer_mutex;
    std::mutex receiver_mutex;

    bool active;
    std::list<std::string> messages_to_broadcast;
    ThreadsafeList messages_delivered;

    bool log;
    bool concat;

public:
    Broadcast(Receiver *receiver, bool log = false);
    virtual ~Broadcast();
    virtual void addLinks(std::vector<Perfectlink *> links_to_add);
    virtual void addMessage(const std::string &msg);
    virtual void setBroadcastActive();
    virtual void setBroadcastInactive();
    virtual void startBroadcast() = 0;
    virtual void broadcastMessage(const std::string &msg) = 0;
    virtual void deliver(const std::string &msg) = 0;
    virtual void setUpperLayer(Broadcast *upper_layer);

    void concatMessages();
    bool getConcat() const;
};