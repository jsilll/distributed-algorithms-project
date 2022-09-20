#pragma once

#include <vector>
#include <string>

#include <netdb.h>

#include <unistd.h>

class Parser
{
public:
  struct Host
  {
    Host();

    Host(size_t id, std::string &ip_or_hostname, unsigned short port);

    in_addr_t ip;
    unsigned long id;
    unsigned short port;

    /**
     * @brief Returns the stringified
     * version of an address
     *
     * @return std::string
     */
    std::string ipReadable() const;

    /**
     * @brief Returns the integer
     * representation of a port
     *
     * @return unsigned short
     */
    unsigned short portReadable() const;

  private:
    /**
     * @brief Resolves an already valid ip address
     *
     * @param host
     * @return in_addr_t
     */
    in_addr_t ipLookup(const char *host);

    /**
     * @brief Checks whether a char* represents
     * valid ip address
     *
     * @param ipAddress
     * @return true
     * @return false
     */
    bool isValidIpAddress(const char *ipAddress);
  };

public:
  Parser(const int argc, char const *const *argv, bool withConfig = true);

  /**
   * @brief Parses all the command line arguments
   *
   */
  void parse();

  /**
   * @brief Returns the id in case the command
   * line arguments have already been parsed.
   *
   * @return unsigned long
   */
  unsigned long id() const;

  /**
   * @brief Returns the hostsPath in case the command
   * line arguments have already been parsed.
   *
   * @return unsigned long
   */
  const char *hostsPath() const;

  /**
   * @brief Returns the outputPath in case the command
   * line arguments have already been parsed.
   *
   * @return unsigned long
   */
  const char *outputPath() const;

  /**
   * @brief Returns the configPath in case the command
   * line arguments have already been parsed.
   *
   * @return unsigned long
   */
  const char *configPath() const;

  /**
   * @brief Returns a vector
   * containing all the hosts in
   * the HOSTS file
   *
   * @return std::vector<Host>
   */
  std::vector<Host> hosts();

private:
  /**
   * @brief Loads the id_ hostsPath_ outputPath_
   * and configPath_ variables
   *
   * @return true
   * @return false
   */
  bool parseInternal();

  /**
   * @brief Prints help info to stderr
   * calls exit(EXIT_FAILURE) after printing
   *
   * @param argv
   */
  void help(const int, char const *const *argv);

  /**
   * @brief Loads the id_ variable
   *
   * @return true
   * @return false
   */
  bool parseID();

  /**
   * @brief Loads the hostsPath_ variable
   *
   * @return true
   * @return false
   */
  bool parseHostPath();

  /**
   * @brief Loads the outputPath_ variable
   *
   * @return true
   * @return false
   */
  bool parseOutputPath();

  /**
   * @brief Loads the configPath_ variable
   *
   * @return true
   * @return false
   */
  bool parseConfigPath();

  // TODO: maybe this shouldn't be here?
  bool isPositiveNumber(const std::string &s) const;

  /**
   * @brief Checks whether the command line arguments have
   * been parsed. Otherwise throws a runtime error.
   *
   */
  void checkParsed() const;

  /**
   * @brief Trims a string on the left side
   *
   * @param s
   */
  void ltrim(std::string &s);

  /**
   * @brief Trims a string on the right side
   *
   * @param s
   */
  void rtrim(std::string &s);

  /**
   * @brief Trims a string on both sides
   *
   * @param s
   */
  void trim(std::string &s);

private:
  const int argc;
  char const *const *argv;
  bool withConfig;
  bool parsed;

  std::string configPath_;
  std::string hostsPath_;
  std::string outputPath_;
  unsigned long id_;
};
