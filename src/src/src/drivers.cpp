#include "drivers.hpp"

#include <iostream>

/**
 * @brief Global scope
 * needed by stop_execution(int)
 *
 */
static std::optional<Logger> logger;

/**
 * @brief UDP Server (Receiver)
 * for this process.
 *
 */
static std::optional<UDPServer> server;

/**
 * @brief UDP Client (Sender)
 * for this process.
 *
 */
static std::optional<UDPClient> client;

/**
 * @brief Stores all the PerfectLinks
 * established during the process's execution.
 *
 */
static std::vector<std::unique_ptr<PerfectLink>> perfect_links;

/**
 * @brief Used for waiting
 * infinitely after sending all
 * messages or setting up all links
 *
 */
static inline void WaitForever()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
}

void drivers::Stop() 
{
  for (const auto &pl : perfect_links) 
  {
    pl->Stop();
  }

  if (server.has_value()) 
  {
    server.value().Stop();
  }

  std::cout << "[INFO] Writing output." << std::endl;

  if (logger.has_value()) 
  {
    logger.value().Flush();
  }
}

void drivers::PerfectLinks(Parser &parser)
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
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        std::exit(EXIT_FAILURE);
    }

    try
    {
        auto local_host = parser.local_host();
        server.emplace(local_host.ip, local_host.port);
        client.emplace(server.value().sockfd());
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

        WaitForever();
    }
    else
    {
        std::cout << "[INFO] Receiving Messages\n";
        std::cout << "[INFO] ==================" << std::endl;

        server.value().Start();

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
                                                            server.value(),
                                                            client.value(),
                                                            logger.value());
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

        WaitForever();
    }
}
