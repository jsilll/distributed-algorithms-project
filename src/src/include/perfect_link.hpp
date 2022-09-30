#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "receiver.hpp"
#include "sender.hpp"
#include "threaded_list.hpp"

class Broadcast;
class PerfectLink;

class PerfectLink
{
private:
    int target_id_;
    int this_process_id_;

    Broadcast &broadcast_;
    std::atomic<bool> link_active_{};

    Sender &sender_;
    Receiver &receiver_;
    std::vector<PerfectLink *> other_links_;

    std::thread ack_thread_;
    std::thread send_thread_;
    std::thread deliver_thread_;

    std::mutex link_mutex_;
    std::mutex receiver_mutex_;
    std::mutex broadcast_mutex_;
    std::mutex acks_to_send_mutex_;
    std::mutex messages_to_send_mutex_;

    std::list<std::string> acks_to_send_;
    std::list<std::string> messages_to_send_;

    ThreadsafeList<std::string> link_delivered_;

public:
    PerfectLink(Sender &sender, Receiver &receiver, Broadcast &broadcast);

    void Ack();
    void Send();
    void Deliver();

    void SendMessage(const std::string &msg);
    void AddMessageRelay(const std::string &msg);

    // ---------- Setters ---------- //

    void active(bool active);
    void add_ack(const std::string &ack);
    void add_message(const std::string &msg);
    void remove_message(const std::string &msg);
    void add_messages(const std::list<std::string> &msgs);
    void add_links(std::vector<PerfectLink *> other_links);
};