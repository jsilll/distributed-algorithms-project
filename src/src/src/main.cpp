#include <chrono>
#include <iostream>
#include <signal.h>
#include <thread>

#include "parser.hpp"
#include "logger.hpp"
#include "udp_server.hpp"
#include "info_display.hpp"

/**
 * @brief Responsible for handling the
 * proper termination of the process
 *
 * @param signum
 */
void stop_execution(int signum);

Logger LOGGER; // needed by stop_execution(int)

int main(int argc, char *argv[])
{
  // Setting up Signal Handling
  signal(SIGINT, stop_execution);
  signal(SIGTERM, stop_execution);
  std::cout << "From a new terminal type `kill -SIGINT " << getpid() << "' or 'kill -SIGTERM " << getpid() << "' to stop processing packets.\n\n";

  // Constructing Parser
  bool require_config = true;
  Parser parser(argc, argv, require_config);

  // Parsing exec args and displaying their values
  parser.Parse();
  display_host_info(parser);
  display_hosts_info(parser);
  display_exec_args_info(parser);

  // Constructing Logger
  LOGGER.Open(parser.output_path());

  // Some Initializations
  std::cout << "Doing some initializations...\n\n";
  Parser::Host localhost = parser.localhost();
  unsigned long n_messages = parser.n_messages();
  unsigned long receiver_id = parser.receiver_id();
  std::cout << "id = " << localhost.id << "; ip = " << localhost.ip_readable() << "; port = " << localhost.port_readable() << std::endl;
  std::cout << "n_messages = " << n_messages << "; receiver_id = " << receiver_id << "\n\n";

  // Broadcasting and Receiving Messages
  std::cout << "Broadcasting and delivering messages..." << std::endl;
  UDPserver server(localhost.ip, localhost.port);

  // After a process finishes broadcasting, it waits forever for the delivery of messages.
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  std::exit(EXIT_SUCCESS);
}

void stop_execution(int signum)
{
  std::cout << signum << " received." << std::endl;

  // Reset Signal Handler To Default
  signal(signum, SIG_DFL);

  // Immediately Stop Network Packet Processing
  std::cout << "Immediately stopping network packet processing." << std::endl;

  // Write/Flush Output File If Necessary
  std::cout << "Writing output." << std::endl;
  LOGGER.Flush();

  // Exiting
  std::exit(EXIT_SUCCESS);
}