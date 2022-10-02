#include <chrono>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <thread>

#include "parser.hpp"
#include "logger.hpp"
#include "drivers.hpp"
#include "udp_server.hpp"
#include "info_display.hpp"

/**
 * @brief Global scope
 * needed by stop_execution(int)
 *
 */
static Logger logger;

/**
 * @brief Responsible for handling the
 * proper termination of the process
 *
 * @param signum
 */
static void stop_execution(int signum)
{
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  std::cout << "\n[INFO] " << strsignal(signum) << " received." << std::endl;
  std::cout << "[INFO] Immediately stopping network packet processing." << std::endl;
  std::cout << "[INFO] Writing output." << std::endl;

  logger.Flush();

  std::exit(EXIT_SUCCESS);
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

  switch (parser.exec_mode())
  {
  case Parser::ExecMode::kPerfectLinks:
    drivers::PerfectLinks(parser.id(),
                          parser.receiver_id(),
                          parser.n_messages(),
                          parser.hosts(),
                          parser.debug(),
                          logger);
    break;
  default:
    std::cerr << "Invalid execution mode." << std::endl;
    std::exit(EXIT_FAILURE);
    break;
  }

  std::exit(EXIT_SUCCESS);
}
