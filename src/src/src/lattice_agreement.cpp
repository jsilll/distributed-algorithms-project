#include "lattice_agreement.hpp"

#include <cmath>
#include <sstream>

void LatticeAgreement::Propose(const std::vector<unsigned int> &proposed) noexcept
{
    std::unordered_set<unsigned int> proposal_set(proposed.begin(), proposed.end());

    current_proposal_state_.mutex.lock_shared();
    bool is_active = current_proposal_state_.data.active;
    current_proposal_state_.mutex.unlock_shared();

    if (is_active)
    {
        to_propose_.mutex.lock();
        to_propose_.data.push(std::move(proposal_set));
        to_propose_.mutex.unlock();
    }
    else
    {
        current_proposal_state_.mutex.lock();
        if (current_round_.load() != 0)
        {
            current_round_.fetch_add(1);
        }
        auto round = current_round_.load();
        current_proposal_state_.data = {{0, std::move(proposal_set)}, true, 1, 0};
        std::vector<char> buffer(kPacketPrefixSize + (proposed.size() * sizeof(unsigned int)));
        std::size_t size = Serialize(Message::Type::kProposal, round, current_proposal_state_.data.proposal.number, current_proposal_state_.data.proposal.values, buffer);

#ifdef DEBUG
        std::cerr << "[DBUG] LatticeAgreement Sending Message of type = " << static_cast<int>(Message::Type::kProposal)
                  << ", round = " << round
                  << ", active_proposal_number = " << current_proposal_state_.data.proposal.number
                  << ", proposal = ";
        for (const auto n : current_proposal_state_.data.proposal.values)
        {
            std::cerr << n << " ";
        }
        std::cerr << std::endl;
#endif
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
        auto message_val = message.value();

#ifdef DEBUG
        std::cerr << "[DBUG] LatticeAgreement Received Message of type = " << static_cast<int>(message_val.type)
                  << ", round = " << message_val.round
                  << ", number = " << message_val.proposal.number
                  << ", proposal = ";
        for (const auto n : message_val.proposal.values)
        {
            std::cerr << n << " ";
        }
        std::cerr << std::endl;
#endif
        std::optional<Broadcast::Message> res{};

        auto round = current_round_.load();
        if (message_val.round == round)
        {
            current_proposal_state_.mutex.lock_shared();
            auto active = current_proposal_state_.data.active;
            current_proposal_state_.mutex.unlock_shared();

            if (active)
            {
#ifdef DEBUG
                std::cerr << "[DBUG] LatticeAgreement Handling in time message\n";
#endif
                current_proposal_state_.mutex.lock();
                res = HandleMessage(message_val, current_proposal_state_.data);
                current_proposal_state_.mutex.unlock();
            }
            else
            {
#ifdef DEBUG
                std::cerr << "[DBUG] LatticeAgreement Handling ahead of time message, proposal not active yet\n";
#endif
                ahead_of_time_messages_.mutex.lock();
                ahead_of_time_messages_.data[message_val.round].push({msg.id.author, message_val});
                ahead_of_time_messages_.mutex.unlock();
            }
        }
        else if (message_val.round > round)
        {
#ifdef DEBUG
            std::cerr << "[DBUG] LatticeAgreement Handling ahead of time message\n";
#endif
            ahead_of_time_messages_.mutex.lock();
            ahead_of_time_messages_.data[message_val.round].push({msg.id.author, message_val});
            ahead_of_time_messages_.mutex.unlock();
        }
        else
        {
            agreed_proposals_.mutex.lock();
            if (agreed_proposals_.data.count(message_val.round))
            {
#ifdef DEBUG
                std::cerr << "[DBUG] LatticeAgreement Handling old message from round = "
                          << message_val.round
                          << " number  = " << message_val.proposal.number << "\n";
#endif
                auto &agreed_proposal = agreed_proposals_.data.at(message_val.round);
                res = HandleMessage(message_val, agreed_proposal);
            }
            agreed_proposals_.mutex.unlock();
        }

        if (res.has_value())
        {
            SendDirectedInternal(res.value(), msg.id.author);
        }
    }
    else
    {
#ifdef DEBUG
        std::cerr << "[DBUG] LatticeAgreement Invalid Message Received\n";
#endif
    }
}

std::optional<Broadcast::Message> LatticeAgreement::HandleMessage(const LatticeAgreement::Message &msg,
                                                                  LatticeAgreement::ProposalState &proposal_state) noexcept
{
    if (msg.type == Message::Type::kAck && proposal_state.proposal.number == msg.proposal.number)
    {
#ifdef DEBUG
        std::cerr << "[DBUG] LatticeAgreement Handling Ack: " << proposal_state.ack_count << "\n";
#endif
        proposal_state.ack_count++;
    }
    else if (msg.type == Message::Type::kNack && proposal_state.proposal.number == msg.proposal.number)
    {
        for (const auto val : msg.proposal.values)
        {
            proposal_state.proposal.values.insert(val);
        }

        proposal_state.nack_count++;
    }
    else if (msg.type == Message::Type::kProposal)
    {
        bool accepted_in_proposed{true};
        for (const auto val : proposal_state.accepted)
        {
            if (msg.proposal.values.count(val) == 0)
            {
                accepted_in_proposed = false;
                break;
            }
        }

        for (const auto val : msg.proposal.values)
        {
            proposal_state.accepted.insert(val);
        }

        if (accepted_in_proposed)
        {
            std::vector<char> buffer(kPacketPrefixSize);
            std::size_t size = Serialize(Message::Type::kAck,
                                         msg.round,
                                         msg.proposal.number,
                                         {},
                                         buffer);

            Broadcast::Message::Id::Seq seq = n_messages_sent_.fetch_add(1);
            Broadcast::Message message = {{seq, id_}, id_, std::move(buffer)};

#ifdef DEBUG
            std::cerr << "[DBUG] LatticeAgreement Sending ACK round = " << msg.round << " proposal_number = " << msg.proposal.number << std::endl;
#endif

            return {message};
        }
        else
        {
            std::vector<char> buffer(kPacketPrefixSize + (proposal_state.accepted.size() * sizeof(unsigned int)));
            std::size_t size = Serialize(Message::Type::kNack,
                                         msg.round,
                                         msg.proposal.number,
                                         proposal_state.accepted,
                                         buffer);

            Broadcast::Message::Id::Seq seq = n_messages_sent_.fetch_add(1);
            Broadcast::Message message = {{seq, id_}, id_, std::move(buffer)};

#ifdef DEBUG
            std::cerr << "[DBUG] LatticeAgreement Sending NAck round = " << msg.round << " proposal_number = " << msg.proposal.number << std::endl;
#endif

            return {message};
        }
    }

    return {};
}

void LatticeAgreement::Decide(std::unordered_set<unsigned int> &values) noexcept
{
    std::stringstream line;
    std::unordered_set<unsigned int>::iterator itr;
    for (itr = values.begin(); itr != values.end(); itr++)
    {
        line << *itr << " ";
    }
    line.seekp(-1, std::ios_base::end);
    logger_ << line.str();
}

void LatticeAgreement::CheckForAgreement() noexcept
{
    while (on_.load())
    {
        agreed_proposals_.mutex.lock();
#ifdef DEBUG
        std::cout << "[DBUG] LatticeAgreement (Threaded) Agreed Proposals Size: "
                  << agreed_proposals_.data.size() << std::endl;
        for (const auto &proposal : agreed_proposals_.data)
        {
            std::cout << "[DBUG] LatticeAgreement (Threaded) Agreed Proposal: "
                      << "round = " << proposal.first << " "
                      << ", number = " << proposal.second.proposal.number << " "
                      << ", ack_count = " << proposal.second.ack_count << " "
                      << ", nack_count = " << proposal.second.nack_count << std::endl;
        }
#endif
        unsigned int last_round = 0;
        for (const auto &[round, proposal_state] : agreed_proposals_.data)
        {
            if (proposal_state.ack_count == n_processes_.load())
            {
                last_round = std::max(last_round, round);
            }
        }
        for (unsigned r = 0; r < last_round; ++r)
        {
            agreed_proposals_.data.erase(r);
        }
        agreed_proposals_.mutex.unlock();

        current_proposal_state_.mutex.lock_shared();
        auto f = std::floor(n_processes_.load() / 2);
        bool decide = current_proposal_state_.data.ack_count > f && current_proposal_state_.data.active;
        bool reset_and_broadcast = current_proposal_state_.data.nack_count > 0 && current_proposal_state_.data.ack_count + current_proposal_state_.data.nack_count > f && current_proposal_state_.data.active;
#ifdef DEBUG
        std::cerr << "[DBUG] LatticeAgreement (Threaded) Checking for agreement: ack_count = "
                  << current_proposal_state_.data.ack_count
                  << " nack_count = " << current_proposal_state_.data.nack_count
                  << " f = " << f
                  << " active = " << current_proposal_state_.data.active << "\n";
#endif
        current_proposal_state_.mutex.unlock_shared();
        if (decide)
        {
#ifdef DEBUG
            std::cerr << "[DBUG] LatticeAgreement (Threaded) Deciding" << std::endl;
#endif
            to_propose_.mutex.lock();
            bool has_new_proposal_to_make = !to_propose_.data.empty();
            auto new_proposal = to_propose_.data.front();
            if (has_new_proposal_to_make)
            {
                to_propose_.data.pop();
            }
            to_propose_.mutex.unlock();

            current_proposal_state_.mutex.lock();
            auto round = current_round_.fetch_add(1) + 1;
            current_proposal_state_.data.active = false;
            auto proposal_state_copy = current_proposal_state_.data;
            if (has_new_proposal_to_make)
            {
                current_proposal_state_.data = {{0, std::move(new_proposal)}, true, 1, 0};
            }
            current_proposal_state_.mutex.unlock();

            ahead_of_time_messages_.mutex.lock();
            auto q = ahead_of_time_messages_.data[round];
            ahead_of_time_messages_.data.erase(round);
            ahead_of_time_messages_.mutex.unlock();

            while (!q.empty())
            {
                auto [author, msg] = q.front();
                q.pop();

#ifdef DEBUG
                std::cerr << "[DBUG] LatticeAgreement (Threaded) Handling Ahead of Time Message type = "
                          << static_cast<int>(msg.type)
                          << ", round = " << msg.round
                          << ", number = " << msg.proposal.number
                          << ", proposal = ";
                for (auto &v : msg.proposal.values)
                {
                    std::cerr << v << " ";
                }
                std::cerr << std::endl;
#endif

                current_proposal_state_.mutex.lock();
                auto res = HandleMessage(msg, current_proposal_state_.data);
                current_proposal_state_.mutex.unlock();

                if (res.has_value())
                {
                    SendDirectedInternal(res.value(), author);
                }
            }

            current_proposal_state_.mutex.lock_shared();
            if (has_new_proposal_to_make)
            {
                std::vector<char> buffer(kPacketPrefixSize + (current_proposal_state_.data.proposal.values.size() * sizeof(unsigned int)));
                std::size_t size = Serialize(Message::Type::kProposal, round, current_proposal_state_.data.proposal.number, current_proposal_state_.data.proposal.values, buffer);
                Broadcast::Message::Id::Seq seq = n_messages_sent_.fetch_add(1);
                Broadcast::Message message = {{seq, id_}, id_, std::move(buffer)};
                SendInternal(message);
            }
            current_proposal_state_.mutex.unlock_shared();

            if (proposal_state_copy.ack_count < n_processes_.load())
            {
#ifdef DEBUG
                std::cerr << "[DBUG] LatticeAgreement (Threaded) Caching round = "
                          << round - 1 << " number = " << proposal_state_copy.proposal.number
                          << std::endl;
#endif
                agreed_proposals_.mutex.lock();
                agreed_proposals_.data.insert({round - 1, proposal_state_copy});
                agreed_proposals_.mutex.unlock();
            }

#ifdef DEBUG
            std::cerr << "[DBUG] LatticeAgreement (Threaded) Actually Deciding" << std::endl;
#endif

            Decide(proposal_state_copy.proposal.values);
        }
        else if (reset_and_broadcast)
        {
#ifdef DEBUG
            std::cerr << "[DBUG] LatticeAgreement (Threaded) Resetting and Broadcasting" << std::endl;
#endif
            current_proposal_state_.mutex.lock();

            current_proposal_state_.data.ack_count = 1;
            current_proposal_state_.data.nack_count = 0;

            ++current_proposal_state_.data.proposal.number;

            std::vector<char> buffer(kPacketPrefixSize + (current_proposal_state_.data.proposal.values.size() * sizeof(unsigned int)));
            std::size_t size = Serialize(Message::Type::kProposal, current_round_.load(), current_proposal_state_.data.proposal.number, current_proposal_state_.data.proposal.values, buffer);

            current_proposal_state_.mutex.unlock();

#ifdef DEBUG
            std::cerr << "[DBUG] LatticeAgreement (Threaded) Sending Proposal type = " << static_cast<int>(Message::Type::kProposal)
                      << ", round = " << current_round_.load()
                      << ", active_proposal_number = " << current_proposal_state_.data.proposal.number
                      << ", number = " << current_proposal_state_.data.proposal.number
                      << ", proposal = ";
            for (auto &v : current_proposal_state_.data.proposal.values)
            {
                std::cerr << v << " ";
            }
            std::cerr << std::endl;
#endif

            Broadcast::Message::Id::Seq seq = n_messages_sent_.fetch_add(1);
            Broadcast::Message message = {{seq, id_}, id_, std::move(buffer)};
            SendInternal(message);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

std::size_t LatticeAgreement::Serialize(Message::Type type,
                                        unsigned int round,
                                        Proposal::Number number,
                                        const std::unordered_set<unsigned int> &values,
                                        std::vector<char> &buffer) noexcept
{
    auto type_ptr = static_cast<const char *>(static_cast<const void *>(&type));
    auto round_ptr = static_cast<const char *>(static_cast<const void *>(&round));
    auto active_proposal_number_ptr = static_cast<const char *>(static_cast<const void *>(&number));

    std::copy(type_ptr, type_ptr + sizeof(Message::Type), buffer.begin());
    std::copy(round_ptr, round_ptr + sizeof(unsigned int), buffer.begin() + sizeof(Message::Type));
    std::copy(active_proposal_number_ptr, active_proposal_number_ptr + sizeof(Proposal::Number), buffer.begin() + sizeof(Message::Type) + sizeof(unsigned int));

    std::size_t index = kPacketPrefixSize;
    for (const auto val : values)
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
    unsigned int round;
    Proposal::Number number;
    std::unordered_set<unsigned int> proposed;

    auto type_ptr = static_cast<char *>(static_cast<void *>(&type));
    auto round_ptr = static_cast<char *>(static_cast<void *>(&round));
    auto number_ptr = static_cast<char *>(static_cast<void *>(&number));

    std::copy(bytes.begin(), bytes.begin() + sizeof(Message::Type), type_ptr);
    std::copy(bytes.begin() + sizeof(Message::Type), bytes.begin() + sizeof(Message::Type) + sizeof(unsigned int), round_ptr);
    std::copy(bytes.begin() + sizeof(Message::Type) + sizeof(unsigned int), bytes.begin() + kPacketPrefixSize, number_ptr);

    for (std::size_t index = kPacketPrefixSize; index < bytes.size(); index += sizeof(unsigned int))
    {
        unsigned int n;
        auto n_ptr = static_cast<char *>(static_cast<void *>(&n));
        std::copy(bytes.begin() + static_cast<std::ptrdiff_t>(index), bytes.begin() + static_cast<std::ptrdiff_t>(index) + sizeof(unsigned int), n_ptr);
        proposed.insert(n);
    }

    return {{type, {number, std::move(proposed)}, round}};
}