#include "lattice_agreement.hpp"

#include <cmath>

void LatticeAgreement::Propose(const std::vector<unsigned int> &proposed) noexcept
{
    std::unordered_set proposal_values(proposed.begin(), proposed.end());

    current_proposal_state_.mutex.lock_shared();
    bool is_active = current_proposal_state_.data.active;
    current_proposal_state_.mutex.unlock_shared();

    if (is_active)
    {
        to_propose_.mutex.lock();
        to_propose_.data.push(std::move(proposal_values));
        to_propose_.mutex.unlock();
    }
    else
    {
        current_proposal_state_.mutex.lock();
        Proposal::Id new_prop_number = current_proposal_state_.data.id + 1;
        current_proposal_state_.data = {new_prop_number, true, 0, 0, std::move(proposal_values)};
        std::vector<char> buffer(kPacketPrefixSize + (proposed.size() * sizeof(unsigned int)));
        std::size_t size = Serialize(Message::Type::kProposal, new_prop_number, current_proposal_state_.data.proposed, buffer);
        current_proposal_state_.mutex.unlock();

        Broadcast::Message::Id::Seq seq = n_messages_sent_.fetch_add(1);
        Broadcast::Message message = {{seq, id_}, id_, std::move(buffer)};
        SendInternal(message);
    }
}

void LatticeAgreement::NotifyInternal(const Broadcast::Message &msg) noexcept
{
    auto message = Parse(msg.payload);

    if (message.has_value())
    {
#ifdef DEBUG
        std::cout << "[DBUG] LatticeAgreement received message of type = " << static_cast<int>(message.value().type)
                  << ", id = " << message.value().proposal.id
                  << ", proposal = ";
        for (const auto n : message.value().proposal.values)
        {
            std::cout << n << " ";
        }
        std::cout << std::endl;
#endif
        current_proposal_state_.mutex.lock();
        auto res = HandleMessage(message.value());
        current_proposal_state_.mutex.unlock();
        if (res.has_value())
        {
            SendDirectedInternal(res.value(), msg.id.author);
        }
#ifdef DEBUG
        current_proposal_state_.mutex.lock_shared();
        std::cout << "[DBUG] LatticeAgreement Current Proposal State active = " << current_proposal_state_.data.active
                  << " ack_count = " << current_proposal_state_.data.ack_count
                  << " nack_count = " << current_proposal_state_.data.nack_count
                  << " id = " << current_proposal_state_.data.id
                  << " proposed = ";
        for (const auto n : current_proposal_state_.data.proposed)
        {
            std::cout << n << " ";
        }
        std::cout << "accepted = ";
        for (const auto n : current_proposal_state_.data.accepted)
        {
            std::cout << n << " ";
        }
        std::cout << "\n";
        current_proposal_state_.mutex.unlock_shared();
#endif
    }
#ifdef DEBUG
    else
    {
        std::cout << "[DBUG] LatticeAgreement Invalid Message Received\n";
    }
#endif
}

std::optional<Broadcast::Message> LatticeAgreement::HandleMessage(const LatticeAgreement::Message &msg) noexcept
{
    // if (msg.proposal.id != current_proposal_state_.data.id)
    // {
    //     // TODO: handle ahead of time messages
    //     return {};
    // }

    if (msg.type == Message::Type::kAck)
    {
        current_proposal_state_.data.ack_count++;
    }
    else if (msg.type == Message::Type::kNack)
    {
        for (const auto val : msg.proposal.values)
        {
            current_proposal_state_.data.proposed.insert(val);
        }

        current_proposal_state_.data.nack_count++;
    }
    else if (msg.type == Message::Type::kProposal)
    {
        bool accepted_in_proposed{true};

        for (const auto val : current_proposal_state_.data.accepted)
        {
            if (msg.proposal.values.count(val) == 0)
            {
                accepted_in_proposed = false;
                break;
            }
        }

        for (const auto val : msg.proposal.values)
        {
            current_proposal_state_.data.accepted.insert(val);
        }

        if (accepted_in_proposed)
        {
            std::vector<char> buffer(kPacketPrefixSize);
            std::size_t size = Serialize(Message::Type::kAck, msg.proposal.id, {}, buffer);

            Broadcast::Message::Id::Seq seq = n_messages_sent_.fetch_add(1);
            Broadcast::Message message = {{seq, id_}, id_, std::move(buffer)};
            return {message};
        }
        else
        {
            std::vector<char> buffer(kPacketPrefixSize + (current_proposal_state_.data.accepted.size() * sizeof(unsigned int)));
            std::size_t size = Serialize(Message::Type::kNack, msg.proposal.id, current_proposal_state_.data.accepted, buffer);

            Broadcast::Message::Id::Seq seq = n_messages_sent_.fetch_add(1);
            Broadcast::Message message = {{seq, id_}, id_, std::move(buffer)};
            return {message};
        }
    }

    return {};
}

void LatticeAgreement::Decide(std::unordered_set<unsigned int> &values) noexcept
{
    std::string line;
    for (const auto n : values)
    {
        line += std::to_string(n) + " ";
    }
    line[line.size()] = '\n';
    logger_ << line;
}

void LatticeAgreement::CheckForAgreement() noexcept
{
    while (on_.load())
    {
        current_proposal_state_.mutex.lock_shared();
        bool decide = current_proposal_state_.data.ack_count > std::floor(n_processes_.load() / 2) &&
                      current_proposal_state_.data.active;
        bool reset_and_broadcast = current_proposal_state_.data.nack_count > 0 &&
                                   current_proposal_state_.data.ack_count + current_proposal_state_.data.nack_count > std::floor(n_processes_.load() / 2) &&
                                   current_proposal_state_.data.active;
        current_proposal_state_.mutex.unlock_shared();

        if (decide)
        {
#ifdef DEBUG
            std::cout << "[DBUG] LatticeAgreements Deciding" << std::endl;
#endif
            current_proposal_state_.mutex.lock();
            current_proposal_state_.data.active = false;
            auto proposed = current_proposal_state_.data.proposed;
            current_proposal_state_.mutex.unlock();
            Decide(proposed);
        }

        if (reset_and_broadcast)
        {
#ifdef DEBUG
            std::cout << "[DBUG] LatticeAgreements Resetting and Broadcasting" << std::endl;
#endif
            current_proposal_state_.mutex.lock();

            current_proposal_state_.data.id++;
            current_proposal_state_.data.ack_count = 0;
            current_proposal_state_.data.nack_count = 0;
            std::vector<char> buffer(kPacketPrefixSize + (current_proposal_state_.data.proposed.size() * sizeof(unsigned int)));
            std::size_t size = Serialize(Message::Type::kProposal, current_proposal_state_.data.id, current_proposal_state_.data.proposed, buffer);

            current_proposal_state_.mutex.unlock();

            Broadcast::Message::Id::Seq seq = n_messages_sent_.fetch_add(1);
            Broadcast::Message message = {{seq, id_}, id_, std::move(buffer)};
            SendInternal(message);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}

std::size_t LatticeAgreement::Serialize(Message::Type type, Proposal::Id id, const std::unordered_set<unsigned int> &proposed, std::vector<char> &buffer) noexcept
{
    auto type_ptr = static_cast<const char *>(static_cast<const void *>(&type));
    auto active_proposal_number_ptr = static_cast<const char *>(static_cast<const void *>(&id));

    std::copy(type_ptr, type_ptr + sizeof(Message::Type), buffer.begin());
    std::copy(active_proposal_number_ptr, active_proposal_number_ptr + sizeof(Proposal::Id), buffer.begin() + sizeof(Message::Type));

    std::size_t index = kPacketPrefixSize;
    for (const auto val : proposed)
    {
        auto val_ptr = static_cast<const char *>(static_cast<const void *>(&val));
        std::copy(val_ptr, val_ptr + sizeof(unsigned int), buffer.begin() + static_cast<std::ptrdiff_t>(index));
        index += sizeof(unsigned int);
    }

    return index;
}

std::optional<LatticeAgreement::Message> LatticeAgreement::Parse(const std::vector<char> &bytes) noexcept
{
    if (bytes.size() < kPacketPrefixSize)
    {
        return {};
    }

    Message::Type type;
    Proposal::Id id;
    std::unordered_set<unsigned int> proposed;

    auto type_ptr = static_cast<char *>(static_cast<void *>(&type));
    auto id_ptr = static_cast<char *>(static_cast<void *>(&id));

    std::copy(bytes.begin(), bytes.begin() + sizeof(Message::Type), type_ptr);
    std::copy(bytes.begin() + sizeof(Message::Type), bytes.begin() + sizeof(Message::Type) + sizeof(Proposal::Id), id_ptr);

    for (std::size_t index = kPacketPrefixSize; index < bytes.size(); index += sizeof(unsigned int))
    {
        unsigned int n;
        auto n_ptr = static_cast<char *>(static_cast<void *>(&n));
        std::copy(bytes.begin() + static_cast<std::ptrdiff_t>(index), bytes.begin() + static_cast<std::ptrdiff_t>(index) + sizeof(unsigned int), n_ptr);
        proposed.insert(n);
    }

    return {{type, {id, std::move(proposed)}}};
}