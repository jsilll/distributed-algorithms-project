#include "drivers.hpp"

#include <iostream>

#include "udp_server.hpp"

void perfect_links_driver(unsigned long n_messages, unsigned long receiver_id, Parser::Host localhost)
{
    std::cout << "PerfectLinks Mode activated." << std::endl;
    std::cout << "Doing some initializations...\n\n";
    std::cout << "n_messages = " << n_messages << "\n";
    std::cout << "receiver_id = " << receiver_id << "\n";
    std::cout << "Broadcasting and delivering messages..." << std::endl;

    try
    {
        std::cout << "id = " << localhost.id << "\n";
        std::cout << "ip = " << localhost.ip_readable() << "\n";
        std::cout << "port = " << localhost.port_readable() << std::endl;

        UDPserver server(localhost.ip, localhost.port);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
}