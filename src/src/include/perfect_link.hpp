#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "broadcast.hpp"
#include "receiver.hpp"
#include "sender.hpp"
#include "threaded_list.hpp"
#include "threaded_queue.hpp"

class Perfectlink
{
private:
    Receiver *receiver;
    Sender *sender;
    Broadcast *broadcast;

    std::vector<Perfectlink *> other_links;

    int target_id;
    int this_process_id;

    std::atomic<bool> link_active;

    std::thread deliver_thread;
    std::thread send_thread;
    std::thread ack_thread;

    std::mutex receiver_mutex;
    std::mutex messages_to_send_mutex;
    std::mutex acks_to_send_mutex;
    std::mutex link_mutex;
    std::mutex broadcast_mutex;

    std::list<std::string> messages_to_send;
    std::list<std::string> acks_to_send;

    ThreadsafeList link_delivered;

public:
    Perfectlink(Receiver *receiver, Sender *sender, Broadcast *broadcast);
    ~Perfectlink();

    void addMessage(const std::string &msg);
    void addMessages(const std::list<std::string> &msgs);
    void addOtherLinks(std::vector<Perfectlink *> other_links);
    void setLinkActive();
    void setLinkInactive();
    void sendNewMessage(const std::string &msg);
    void sendThreaded();
    void deliverThreaded();
    void ackThreaded();
    void addAck(const std::string &ack);
    void removeMessage(const std::string &msg);
    void addMessageRelay(const std::string &msg);
};