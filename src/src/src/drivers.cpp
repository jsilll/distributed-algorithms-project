#include "drivers.hpp"

#include <iostream>
#include <optional>

#include "udp_server.hpp"
#include "udp_client.hpp"
#include "perfect_link.hpp"

/**
 * @brief Used for waiting
 * infinitely after sending all
 * messages or setting up all links
 *
 */
static void wait_forever(void)
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
}

void drivers::PerfectLinks(const unsigned long int id, const unsigned long int target_id, const unsigned long n_messages, const std::vector<Parser::Host> &hosts, Logger &logger)
{
    Parser::Host localhost = hosts[id - 1];
    Parser::Host target_host = hosts[target_id - 1];

    std::cout << "[INFO] PerfectLinks Mode Activated\n";
    std::cout << "[INFO] ===========================\n";
    std::cout << "[INFO] n_messages = " << n_messages << "\n";
    std::cout << "[INFO] target_id = " << target_host.id << "\n";
    std::cout << "[INFO] id = " << localhost.id << "\n";
    std::cout << "[INFO] ip = " << localhost.ip_readable() << "\n";
    std::cout << "[INFO] port = " << localhost.port_readable() << std::endl;

    std::optional<UDPServer> server;
    std::optional<UDPClient> client;

    try
    {
        server.emplace(localhost.ip, localhost.port);
        client.emplace(server.value());
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        std::exit(EXIT_FAILURE);
    }

    if (id != target_id)
    {
        std::optional<PerfectLink> pl;

        try
        {
            pl.emplace(id,
                       target_host.id,
                       target_host.ip,
                       target_host.port,
                       server.value(),
                       client.value(),
                       logger,
                       true);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            std::exit(EXIT_FAILURE);
        }

        std::cout << "[INFO] Sending Messages ..." << std::endl;

        for (unsigned long i = 0; i < n_messages; ++i)
        {
            pl.value().Send(std::to_string(i));
        }

        wait_forever();
    }
    else
    {
        std::vector<std::unique_ptr<PerfectLink>> pls;

        std::cout << "[INFO] Receiving Messages ..." << std::endl;

        for (auto &peer : hosts)
        {
            if (id != peer.id)
            {
                try
                {
                    pls.push_back(std::make_unique<PerfectLink>(id,
                                                                peer.id,
                                                                peer.ip,
                                                                peer.port,
                                                                server.value(),
                                                                client.value(),
                                                                logger,
                                                                true));
                }
                catch (const std::exception &e)
                {
                    std::cerr << e.what() << '\n';
                    std::exit(EXIT_FAILURE);
                }
            }
        }

        wait_forever();
    }
}
