#include "drivers.hpp"

#include <iostream>

/**
 * @brief Used for waiting
 * infinitely after sending all
 * messages or setting up all links
 *
 */
static void wait_forever()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
}

void drivers::PerfectLinks(Parser &parser,
                           Logger &logger,
                           UDPServer &server,
                           UDPClient &client,
                           std::vector<std::unique_ptr<PerfectLink>> &perfect_links)
{
    auto id = parser.id();
    auto hosts = parser.hosts();
    auto n_messages = parser.n_messages();
    auto local_host = parser.local_host();
    auto target_host = parser.target_host();

    std::cout << "[INFO] PerfectLinks Mode Activated\n";
    std::cout << "[INFO] ===========================\n";
    std::cout << "[INFO] n_messages = " << n_messages << "\n";
    std::cout << "[INFO] target_id = " << target_host.id << "\n";
    std::cout << "[INFO] id = " << local_host.id << "\n";
    std::cout << "[INFO] ip = " << local_host.ip_readable() << "\n";
    std::cout << "[INFO] port = " << local_host.port_readable() << std::endl;

    if (id != target_host.id)
    {
        server.Start();

        try
        {
            auto pl = std::make_unique<PerfectLink>(id,
                                                    target_host.id,
                                                    target_host.ip,
                                                    target_host.port,
                                                    server,
                                                    client,
                                                    logger);
            pl->Start();
            perfect_links.push_back(std::move(pl));
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            std::exit(EXIT_FAILURE);
        }

        std::cout << "[INFO] Sending Messages\n";
        std::cout << "[INFO] ================" << std::endl;

        for (unsigned long i = 0; i < n_messages; ++i)
        {
            for (const auto &pl : perfect_links)
            {
                pl->Send(std::to_string(i));
            }
        }

        wait_forever();
    }
    else
    {
        std::cout << "[INFO] Receiving Messages\n";
        std::cout << "[INFO] ==================" << std::endl;

        server.Start();

        for (const auto peer : hosts)
        {
            if (id != peer.id)
            {
                try
                {
                    auto pl = std::make_unique<PerfectLink>(id,
                                                            peer.id,
                                                            peer.ip,
                                                            peer.port,
                                                            server,
                                                            client,
                                                            logger);
                    pl->Start();
                    perfect_links.push_back(std::move(pl));
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
