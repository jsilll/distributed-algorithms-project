#include <csignal>
#include <cstring>
#include <iostream>
#include <optional>

#include "parser.hpp"
#include "drivers.hpp"
#include "udp_client.hpp"
#include "info_display.hpp"

static void StopExecution(int signum)
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
  signal(SIGINT, StopExecution);
  signal(SIGTERM, StopExecution);

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
  case Parser::ExecMode::kLatticeAgreement:
    drivers::LatticeAgreement(parser);
    break;
  default:
    std::cerr << "Invalid execution mode." << std::endl;
    break;
  }

  std::exit(EXIT_FAILURE);
}
