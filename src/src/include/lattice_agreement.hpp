#pragma once

#include "logger.hpp"
#include "best_effort_broadcast.hpp"

#include <queue>

class LatticeAgreement final : public BestEffortBroadcast
{
private:
  struct Proposal
  {
    typedef unsigned int Number;
    Number number;
    std::unordered_set<unsigned int> values;
  };

  struct Message
  {
    enum Type
    {
      kProposal,
      kAck,
      kNack,
    } type;
    Proposal proposal;
    unsigned int round;
  };

  struct ProposalState
  {
    bool active{false};
    unsigned int ack_count{0};
    unsigned int nack_count{0};
    Proposal::Number active_proposal_number{0};
    std::unordered_set<unsigned int> proposed{};
    std::unordered_set<unsigned int> accepted{};
  };

public:
  static constexpr int kPacketPrefixSize = sizeof(Message::Type) + sizeof(unsigned int) + sizeof(Proposal::Number);

private:
  std::atomic_uint round_{0};
  std::thread agreement_checker_thread_;
  Shared<ProposalState> current_proposal_state_{};
  Shared<std::queue<std::unordered_set<unsigned int>>> to_propose_{};

public:
  LatticeAgreement(Logger &logger, PerfectLink::Id id) noexcept
      : BestEffortBroadcast::BestEffortBroadcast(logger, id) {}

  ~LatticeAgreement() noexcept override = default;

  inline void Start() noexcept override
  {
    Broadcast::Start();
#ifdef DEBUG
    std::cout << "[DBUG] Creating new thread: LatticeAgreement::CheckForAgreement\n";
#endif
    agreement_checker_thread_ = std::thread(&LatticeAgreement::CheckForAgreement, this);
  }

  inline void Stop() noexcept override
  {
    if (on_.load())
    {
      Broadcast::Stop();
      agreement_checker_thread_.join();
    }
  }

  void Propose(const std::vector<unsigned int> &proposed) noexcept;

  void NotifyInternal(const Broadcast::Message &msg) noexcept override;

private:
  void CheckForAgreement() noexcept;

  void Decide(std::unordered_set<unsigned int> &values) noexcept;

  std::optional<Broadcast::Message> HandleMessage(const Message &msg) noexcept;

  static std::optional<Message> Parse(const std::vector<char> &bytes) noexcept;

  static std::size_t Serialize(Message::Type type, unsigned int round, Proposal::Number id, const std::unordered_set<unsigned int> &proposed, std::vector<char> &buffer) noexcept;
};