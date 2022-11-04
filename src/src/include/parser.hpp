#pragma once

#include <fstream>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <vector>

class Parser
{
public:
  struct Host
  {

    in_addr_t ip;
    unsigned long id;
    unsigned short port;

    Host(size_t id, std::string &ip_or_hostname, unsigned short port);

    [[nodiscard]] std::string ip_readable() const;
    [[nodiscard]] unsigned short port_readable() const;

  private:
    static in_addr_t IpLookup(const char *host);
    static bool IsValidAddress(const char *ipAddress);
  };

  enum ExecMode
  {
    kPerfectLinks,
    kFIFOBroadcast,
  };

private:
  const int argc_;
  char const *const *argv_;
  bool requires_config_;
  bool parsed_;

  std::string config_path_;
  std::string hosts_path_;
  std::string output_path_;

  std::vector<Host> hosts_;

  unsigned long id_{};
  unsigned long n_messages_sent_{};
  unsigned long receiver_id_{};

  ExecMode exec_mode_{kFIFOBroadcast};

public:
  Parser(int argc, char const *const *argv, bool requires_config = true);

  void Parse();

  [[nodiscard]] bool requires_config() const noexcept;
  [[nodiscard]] std::string config_path() const;
  [[nodiscard]] std::string hosts_path() const;
  [[nodiscard]] std::string output_path() const;
  [[nodiscard]] std::vector<Host> hosts() const;
  [[nodiscard]] unsigned long id() const;
  [[nodiscard]] unsigned long n_messages() const;
  [[nodiscard]] unsigned long target_id() const;
  [[nodiscard]] ExecMode exec_mode() const noexcept;
  [[nodiscard]] Host local_host() const;
  [[nodiscard]] Host target_host() const;

private:
  void CheckParsed() const;
  void Help(int, char const *const *argv) const;

  bool ParseInternal();

  bool ParseId() noexcept;
  void ParseMode();
  bool ParseHostPath() noexcept;
  bool ParseOutputPath() noexcept;
  bool ParseConfigPath() noexcept;
  void ParseHostsFile();
  void ParseConfigFile();
};
