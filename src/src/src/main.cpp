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
 * @brief Responsible for handling the
 * proper termination of the process
 *
 * @param signum
 */
void stop_execution(int signum);

/**
 * @brief Needed by stop_execution(int)
 *
 */
Logger logger;

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

  display_host_info(parser);
  display_hosts_info(parser);
  display_exec_args_info(parser);

  try
  {
    logger.Open(parser.output_path());
  }
  catch (const std::ofstream::failure &e)
  {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }

  switch (parser.exec_mode())
  {
  case Parser::ExecMode::kPerfectLinks:
    try
    {
      perfect_links_driver(parser.id(), parser.receiver_id(), parser.n_messages(), parser.hosts(), logger);
    }
    catch (const std::exception &e)
    {
      std::cerr << e.what() << '\n';
      std::exit(EXIT_FAILURE);
    }
    break;
  default:
    std::cerr << "Invalid execution mode." << std::endl;
    std::exit(EXIT_FAILURE);
    break;
  }

  std::exit(EXIT_SUCCESS);
}

void stop_execution(int signum)
{
  signal(SIGINT, stop_execution);
  signal(SIGTERM, stop_execution);

  std::cout << "[INFO " << strsignal(signum) << " received." << std::endl;
  std::cout << "[INFO] Immediately stopping network packet processing." << std::endl;
  std::cout << "[INFO] Writing output." << std::endl;

  logger.Flush();

  std::exit(EXIT_SUCCESS);
}