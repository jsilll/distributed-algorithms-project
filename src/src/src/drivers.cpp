#include "drivers.hpp"

#include <iostream>

static std::optional<Logger> logger;
static std::optional<UDPServer> server;
static std::optional<UDPClient> client;
static std::unique_ptr<PerfectLink::BasicManager> pl_manager;

static inline void WaitForever() noexcept
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
}

void drivers::StopExecution() noexcept
{
    if (server.has_value())
    {
        // Stop receiving Messages
        server.value().Stop();
    }

    // Stop sending Messages
    pl_manager->Stop();

    std::cout << "[INFO] Writing output." << std::endl;

    if (logger.has_value())
    {
        logger.value().Flush();
    }
}

void drivers::PerfectLinks(Parser &parser) noexcept
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

    try
    {
        logger.emplace(parser.output_path());
        server.emplace(local_host.ip, local_host.port);
        client.emplace(server.value().sockfd());
        pl_manager = std::make_unique<PerfectLink::BasicManager>();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        std::exit(EXIT_FAILURE);
    }

    if (id != target_host.id)
    {
        server.value().Start();

        try
        {
            auto pl = std::make_unique<PerfectLink>(id,
                                                    target_host.id,
                                                    target_host.ip,
                                                    target_host.port,
                                                    server.value(),
                                                    client.value(),
                                                    logger.value());
            pl_manager->Add(std::move(pl));
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            std::exit(EXIT_FAILURE);
        }

        pl_manager->Start();

        std::cout << "[INFO] Sending Messages\n";
        std::cout << "[INFO] ================" << std::endl;

        for (unsigned long i = 0; i < n_messages; ++i)
        {
            pl_manager->Send(target_host.id, std::to_string(i));
        }
    }
    else
    {
        std::cout << "[INFO] Receiving Messages\n";
        std::cout << "[INFO] ==================" << std::endl;

        server.value().Start();

        for (const auto &peer : hosts)
        {
            if (id != peer.id)
            {
                try
                {
                    auto pl = std::make_unique<PerfectLink>(id,
                                                            peer.id,
                                                            peer.ip,
                                                            peer.port,
                                                            server.value(),
                                                            client.value(),
                                                            logger.value());
                    pl_manager->Add(std::move(pl));
                }
                catch (const std::exception &e)
                {
                    std::cerr << e.what() << '\n';
                    std::exit(EXIT_FAILURE);
                }
            }
        }

        pl_manager->Start();
    }

    WaitForever();
}
