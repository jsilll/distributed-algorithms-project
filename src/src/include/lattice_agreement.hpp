#pragma once

#include "logger.hpp"
#include "best_effort_broadcast.hpp"

class LatticeAgreement final : public BestEffortBroadcast
{
private:
  struct Packet
  {
    enum Type
    {
      kAck,
      kNack,
      kProposal,
    };

    Type type;
    unsigned int active_proposal_number;
    std::unordered_set<unsigned int> proposed;
  };

  static constexpr int kPacketPrefixSize = sizeof(Packet::Type) + sizeof(unsigned int);

  struct ProposerState
  {
    bool active{false};
    unsigned int ack_count{0};
    unsigned int nack_count{0};
    unsigned int active_proposal_number{0};
    std::unordered_set<unsigned int> proposed;
  };

private:
  PerfectLink::Id id_;
  std::unordered_map<PerfectLink::Id, ProposerState> proposers_;

public:
  LatticeAgreement(Logger &logger, PerfectLink::Id id) noexcept
      : BestEffortBroadcast::BestEffortBroadcast(logger, id)
  {
  }

  ~LatticeAgreement() noexcept override = default;

  void Propose([[maybe_unused]] const std::vector<unsigned int> &proposed) noexcept
  {
    auto &state = proposers_[id_];

    state.active = true;
    state.ack_count = 0;
    state.nack_count = 0;
    ++state.active_proposal_number;
    state.proposed = std::unordered_set(proposed.begin(), proposed.end());

    std::vector<char> buffer(kPacketPrefixSize + proposed.size());
    std::size_t size = Serialize(Packet::Type::kProposal, state.active_proposal_number, state.proposed, buffer);

    Broadcast::Message::Id::Seq seq = n_messages_sent_.fetch_add(1);
    Broadcast::Message message = {{seq, id_}, id_, std::move(buffer)};
    SendInternal(message);
  }

  void NotifyInternal(const Broadcast::Message &msg) noexcept override
  {
    auto message = Parse(msg.payload);

    if (message.has_value())
    {
#ifdef DEBUG
      std::cout << "LatticeAgreement Received Message type: " << static_cast<int>(message.value().type)
                << " active_proposal_number: " << message.value().active_proposal_number << std::endl;
#endif
    }
  }

  static std::size_t Serialize(Packet::Type type, unsigned int active_proposal_number, const std::unordered_set<unsigned int> &proposed, std::vector<char> &buffer) noexcept
  {
    auto type_ptr = static_cast<const char *>(static_cast<const void *>(&type));
    auto active_proposal_number_ptr = static_cast<const char *>(static_cast<const void *>(&active_proposal_number));

    std::copy(type_ptr, type_ptr + sizeof(Packet::Type), buffer.begin());
    std::copy(active_proposal_number_ptr, active_proposal_number_ptr + sizeof(unsigned int), buffer.begin() + sizeof(Packet::Type));

    std::size_t index = kPacketPrefixSize;
    for (const auto &val : proposed)
    {
      auto val_ptr = static_cast<const char *>(static_cast<const void *>(&val));
      std::copy(val_ptr, val_ptr + sizeof(unsigned int), buffer.begin() + static_cast<std::ptrdiff_t>(index));
      index += sizeof(unsigned int);
    }

    return index;
  }

  static std::optional<Packet> Parse([[maybe_unused]] const std::vector<char> &bytes) noexcept
  {
    if (bytes.size() < kPacketPrefixSize)
    {
      return {};
    }

    Packet::Type type;
    unsigned int active_proposal_number;
    std::unordered_set<unsigned int> proposed;

    auto type_ptr = static_cast<char *>(static_cast<void *>(&type));
    auto active_proposal_number_ptr = static_cast<char *>(static_cast<void *>(&active_proposal_number));

    std::copy(bytes.begin(), bytes.begin() + sizeof(Packet::Type), type_ptr);
    std::copy(bytes.begin() + sizeof(Packet::Type), bytes.begin() + kPacketPrefixSize, active_proposal_number_ptr);
    std::copy(bytes.begin() + kPacketPrefixSize, bytes.end(), std::inserter(proposed, proposed.begin()));

    return {{type, active_proposal_number, proposed}};
  }
};