#pragma once

#include <netdb.h>
#include <string>
#include <unistd.h>
#include <vector>

class Parser
{
public:
  struct Host
  {
    Host(size_t id, std::string &ip_or_hostname, unsigned short port);

    in_addr_t ip;
    unsigned long id;
    unsigned short port;

    // -- Getters --

    std::string ip_readable() const;
    unsigned short port_readable() const;

  private:
    in_addr_t IpLookup(const char *host);
    bool IsValidAddress(const char *ipAddress);
  };

public:
  Parser(const int argc, char const *const *argv, bool requires_config = true);

  void Parse();

  // -- Getters --

  bool requires_config() const;
  std::string config_path() const;
  std::string hosts_path() const;
  std::string output_path() const;
  std::vector<Host> hosts() const;
  unsigned long id() const;
  unsigned long n_messages() const;
  unsigned long receiver_id() const;
  Host localhost() const;

private:
  void CheckParsed() const;
  void Help(const int, char const *const *argv) const;

  bool ParseInternal();

  bool ParseId();
  bool ParseHostPath();
  bool ParseOutputPath();
  bool ParseConfigPath();
  void ParseHostsFile();
  void ParseConfigFile();

private:
  const int argc_;
  char const *const *argv_;
  bool requires_config_;
  bool parsed_;

  std::string config_path_;
  std::string hosts_path_;
  std::string output_path_;

  std::vector<Host> hosts_;

  unsigned long id_;
  unsigned long n_messages_;
  unsigned long receiver_id_;
};
