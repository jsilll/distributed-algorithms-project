#include <chrono>
#include <iostream>
#include <signal.h>
#include <thread>

#include "parser.hpp"
#include "logger.hpp"

static void stop_execution(int);
static void display_host_info(Parser &);
static void display_hosts_info(Parser &);
static void display_exec_args_info(Parser &);

Logger LOGGER;

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
  LOGGER.open(parser.output_path());

  std::cout << "Doing some initializations...\n\n";
  Parser::Host localhost = parser.localhost();
  unsigned long n_messages = parser.n_messages();
  unsigned long receiver_id = parser.receiver_id();
  std::cout << "id = " << localhost.id << "; ip = " << localhost.ip_readable() << "; port = " << localhost.port_readable() << std::endl;
  std::cout << "n_messages = " << n_messages << "; receiver_id = " << receiver_id << "\n\n";
  std::cout << "Broadcasting and delivering messages..." << std::endl;

  // After a process finishes broadcasting, it waits forever for the delivery of messages.
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  std::exit(EXIT_SUCCESS);
}

/**
 * @brief Responsible for handling the
 * proper termination of the process
 *
 * @param signum
 */
static void stop_execution(int signum)
{
  std::cout << signum << " received." << std::endl;

  // Reset Signal Handler To Default
  signal(signum, SIG_DFL);

  // Immediately Stop Network Packet Processing
  std::cout << "Immediately stopping network packet processing." << std::endl;

  // Write/Flush Output File If Necessary
  std::cout << "Writing output." << std::endl;
  LOGGER.flush();

  // Exiting
  std::exit(EXIT_SUCCESS);
}

/**
 * @brief Displays the process's information
 *
 * @param parser
 */
static void display_host_info(Parser &parser)
{
  std::cout << "My ID: " << parser.id() << "\n";
  std::cout << "My PID: " << getpid() << "\n";
}

/**
 * @brief Displays the information about
 * the other processes' on the network
 *
 * @param parser
 */
static void display_hosts_info(Parser &parser)
{
  std::cout << "List of resolved hosts is:\n";
  std::cout << "==========================\n";
  auto hosts = parser.hosts();
  for (auto &host : hosts)
  {
    std::cout << host.id << "\n";
    std::cout << "Human-readable IP: " << host.ip_readable() << "\n";
    std::cout << "Machine-readable IP: " << host.ip << "\n";
    std::cout << "Human-readbale Port: " << host.port_readable() << "\n";
    std::cout << "Machine-readbale Port: " << host.port << "\n";
    std::cout << "\n";
  }
  std::cout << "\n";
}

/**
 * @brief Displays information about the
 * execution arguments
 *
 * @param parser
 */
static void display_exec_args_info(Parser &parser)
{
  std::cout << "Path to hosts:\n";
  std::cout << "===============\n";
  std::cout << parser.hosts_path() << "\n\n";
  std::cout << "Path to output:\n";
  std::cout << "===============\n";
  std::cout << parser.output_path() << "\n\n";
  if (parser.requires_config())
  {
    std::cout << "Path to config:\n";
    std::cout << "===============\n";
    std::cout << parser.config_path() << "\n\n";
  }
}