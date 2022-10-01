#include "drivers.hpp"

#include <iostream>

#include "udp_server.hpp"
#include "udp_client.hpp"
#include "perfect_link.hpp"

void perfect_links_driver(const unsigned long int id, const unsigned long int target_id, const unsigned long n_messages, const std::vector<Parser::Host> &hosts, Logger &logger)
{
    Parser::Host localhost = hosts[id - 1];
    Parser::Host target_host = hosts[target_id - 1];

    std::cout << "[INFO] === PerfectLinks Mode Activated ===\n";
    std::cout << "[INFO] n_messages = " << n_messages << "\n";
    std::cout << "[INFO] target_id = " << target_host.id << "\n";
    std::cout << "[INFO] id = " << localhost.id << "\n";
    std::cout << "[INFO] ip = " << localhost.ip_readable() << "\n";
    std::cout << "[INFO] port = " << localhost.port_readable() << std::endl;

    UDPServer server(localhost.ip, localhost.port);
    UDPClient client(server);

    if (id != target_id)
    {
        std::cout << "[INFO] Sending Messages ..." << std::endl;

        PerfectLink pl(id, target_host.id, target_host.ip, target_host.port, server, client, logger);

        for (unsigned long i = 0; i < n_messages; ++i)
        {
            pl.Send(std::to_string(i));
        }

        // while (true)
        // {
        //     std::this_thread::sleep_for(std::chrono::hours(1));
        // }
    }
    else
    {
        std::cout << "[INFO] Receiving Messages ..." << std::endl;
        std::vector<std::unique_ptr<PerfectLink>> pls;
        for (auto &peer : hosts)
        {
            if (id != peer.id)
            {
                pls.push_back(std::make_unique<PerfectLink>(id, peer.id, peer.ip, peer.port, server, client, logger));
            }
        }

        // while (true)
        // {
        //     std::this_thread::sleep_for(std::chrono::hours(1));
        // }
    }
}