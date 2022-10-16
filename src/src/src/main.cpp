#include <csignal>
#include <cstring>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>

#include "drivers.hpp"
#include "perfect_link.hpp"
#include "info_display.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "udp_client.hpp"
#include "udp_server.hpp"

/**
 * @brief Global scope
 * needed by stop_execution(int)
 *
 */
static Logger logger;

/**
 * @brief UDP Server (Receiver)
 * for this process.
 *
 */
static std::optional<UDPServer> server;

/**
 * @brief UDP Client (Sender)
 * for this process.
 *
 */
static std::optional<UDPClient> client;

/**
 * @brief Stores all the PerfectLinks
 * established during the process's execution.
 *
 */
static std::vector<std::unique_ptr<PerfectLink>> perfect_links;

/**
 * @brief Responsible for handling the
 * proper termination of the process
 *
 * @param signum
 */
static void stop_execution(int signum)
{
  signal(SIGINT, SIG_DFL);
  signal(SIGTERM, SIG_DFL);

  std::cout << "\n[INFO] " << strsignal(signum) << " received.\n";
  std::cout << "[INFO] Immediately stopping network packet processing.\n";

  for (const auto &pl : perfect_links) 
  {
    pl->Stop();
  }

  if (server.has_value()) 
  {
    server.value().Stop();
  }

  std::cout << "[INFO] Writing output." << std::endl;

  logger.Flush();

  std::_Exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
  signal(SIGINT, stop_execution);
  signal(SIGTERM, stop_execution);

  Parser parser(argc, argv, true);

  try
  {
    parser.Parse();
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }

  info_display::Localhost(parser);
  info_display::Hosts(parser);
  info_display::ExecArgs(parser);

  try
  {
    logger.Open(parser.output_path());
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }

  try
  {
    auto local_host = parser.local_host();
    server.emplace(local_host.ip, local_host.port);
    client.emplace(server.value().sockfd());
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }

  switch (parser.exec_mode())
  {
  case Parser::ExecMode::kPerfectLinks:
    drivers::PerfectLinks(parser, logger, server.value(), client.value(), perfect_links);
    break;
  default:
    std::cerr << "Invalid execution mode." << std::endl;
    std::exit(EXIT_FAILURE);
    break;
  }

  std::exit(EXIT_SUCCESS);
}
