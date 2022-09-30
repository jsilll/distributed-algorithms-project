#include "broadcast.hpp"

Broadcast::Broadcast(Logger &logger, Receiver &receiver)
    : logger_(logger), receiver_(receiver)
{
}

void Broadcast::ConcatMessages()
{
    concat_ = true;
    unsigned long msg_num{};
    unsigned long group_num{};
    unsigned long group_by = 15;
    unsigned long num_concat = messages_to_broadcast_.size() / group_by;
    unsigned long remainder = messages_to_broadcast_.size() % group_by;

    std::list<std::string> messages_concat;
    std::string message_concatenated = std::to_string(group_by); // number of messages we concat in msg
    for (auto &message : messages_to_broadcast_)
    {
        if (group_num < num_concat)
        {
            if (msg_num == group_by - 1)
            {
                // don't add sep for last message to concat
                message_concatenated += message;
            }
            else
            {

                message_concatenated += message + 's';
            }

            ++msg_num;

            // If we concatenated group_by messages,
            // push concat and create new concatenated msg
            if (msg_num == group_by)
            {
                ++group_num;
                msg_num = 0;
                messages_concat.push_back(message_concatenated);
                message_concatenated = std::to_string(group_by);
            }
        }
        else if ((group_num == num_concat) & (remainder > 0))
        {
            if (msg_num == 0)
            {

                char str_remainder[3];
                sprintf(str_remainder, "%02lu", remainder);

                message_concatenated = std::string(str_remainder);
            }

            if (msg_num == remainder - 1)
            {
                // don't add sep for last message to concat
                message_concatenated += message;
                messages_concat.push_back(message_concatenated);
            }
            else
            {

                message_concatenated += message + 's';
            }

            ++msg_num;
        }
    }

    messages_to_broadcast_ = messages_concat;
}

// ---------- Getters ---------- //

bool Broadcast::concat() const
{
    return concat_;
}

// ---------- Setters ---------- //

void Broadcast::active(bool active)
{
    active_ = active;

    // TODO: ??
    for (auto &link : links_)
    {
        if (link != nullptr)
        {
            link->active(active);
        }
    }
}

void Broadcast::upper_layer(Broadcast *upper_layer)
{
    this->upper_layer_ = upper_layer;
}

void Broadcast::links(std::vector<PerfectLink *> links_to_add)
{
    links_ = links_to_add;
}

void Broadcast::add_message_to_broadcast(const std::string &msg)
{
    messages_to_broadcast_.push_back(msg);
}

// ---------- Protected ---------- //

void Broadcast::AddSentMessageLog(const std::string &msg)
{
    logger_ << 'b' + msg;
}

void Broadcast::AddDelieveredMessageLog(const std::string &msg)
{
    logger_ << 'd' + msg;
}