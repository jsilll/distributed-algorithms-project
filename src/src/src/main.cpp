#include <thread>
#include <vector>
#include <csignal>
#include <cstring>
#include <iostream>
#include <optional>

#include "logger.hpp"
#include "parser.hpp"
#include "drivers.hpp"
#include "udp_client.hpp"
#include "udp_server.hpp"
#include "perfect_link.hpp"
#include "info_display.hpp"

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

  std::cout << "\n[INFO] " << strsignal(signum) << " signal received.\n";
  std::cout << "[INFO] Immediately stopping network packet processing.\n";

  drivers::StopExecution();

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

  switch (parser.exec_mode())
  {
  case Parser::ExecMode::kPerfectLinks:
    drivers::PerfectLinks(parser);
    break;
  case Parser::ExecMode::kFIFOBroadcast:
    drivers::FIFOBroadcast(parser);
    break;
  default:
    std::cerr << "Invalid execution mode." << std::endl;
    break;
  }

  std::exit(EXIT_FAILURE);
}
