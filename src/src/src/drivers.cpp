#include "drivers.hpp"

#include <iostream>

#include "fifo_broadcast.hpp"
#include "lattice_agreement.hpp"

static std::optional<Logger> logger;
static std::optional<UDPServer> server;
static std::optional<UDPClient> client;
static std::unique_ptr<PerfectLink::Manager> manager;

[[noreturn]] static inline void WaitForever() noexcept
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
}

void drivers::StopExecution() noexcept
{
    if (manager != nullptr)
    {
        manager->Stop();
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

void drivers::PerfectLinks(Parser &parser) noexcept
{
    auto id = parser.id();
    auto hosts = parser.hosts();
    auto n_messages = parser.n_messages_to_send();
    auto local_host = parser.local_host();
    auto target_host = parser.target_host();

    std::cout << "[INFO] PerfectLinks Mode Activated\n"
              << "[INFO] ===========================\n"
              << "[INFO] n_messages = " << n_messages << "\n"
              << "[INFO] target_id = " << target_host.id << "\n"
              << "[INFO] id = " << local_host.id << "\n"
              << "[INFO] ip = " << local_host.ip_readable() << "\n"
              << "[INFO] port = " << local_host.port_readable() << std::endl;

    try
    {
        logger.emplace(parser.output_path());
        server.emplace(local_host.ip, local_host.port);
        client.emplace(server.value().sockfd());
        manager = std::make_unique<PerfectLink::BasicManager>(logger.value());
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
                                                    client.value());
            manager->Add(std::move(pl));
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            std::exit(EXIT_FAILURE);
        }

        manager->Start();

        std::cout << "[INFO] Sending Messages\n"
                  << "[INFO] ================" << std::endl;

        auto basic_manager = dynamic_cast<PerfectLink::BasicManager *>(manager.get());
        for (unsigned int i = 0; i < n_messages; ++i)
        {
            basic_manager->Send(target_host.id, "");
        }
    }
    else
    {
        std::cout << "[INFO] Receiving Messages\n"
                  << "[INFO] ==================" << std::endl;

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
                                                            client.value());
                    manager->Add(std::move(pl));
                }
                catch (const std::exception &e)
                {
                    std::cerr << e.what() << '\n';
                    std::exit(EXIT_FAILURE);
                }
            }
        }

        manager->Start();
    }

    WaitForever();
}

void drivers::FIFOBroadcast(Parser &parser) noexcept
{
    auto id = parser.id();
    auto hosts = parser.hosts();
    auto n_messages = parser.n_messages_to_send();
    auto local_host = parser.local_host();

    std::cout << "[INFO] FIFO Broadcast Mode Activated\n"
              << "[INFO] ===========================\n"
              << "[INFO] n_messages = " << n_messages << "\n"
              << "[INFO] id = " << local_host.id << "\n"
              << "[INFO] ip = " << local_host.ip_readable() << "\n"
              << "[INFO] port = " << local_host.port_readable() << std::endl;

    try
    {
        logger.emplace(parser.output_path(), true);
        server.emplace(local_host.ip, local_host.port);
        client.emplace(server.value().sockfd());
        manager = std::make_unique<UniformFIFOBroadcast>(logger.value(), id);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        std::exit(EXIT_FAILURE);
    }

    auto fifo = dynamic_cast<UniformFIFOBroadcast *>(manager.get());

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
                                                        client.value());
                fifo->Add(std::move(pl));
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
                std::exit(EXIT_FAILURE);
            }
        }
    }

    server.value().Start();
    fifo->Start();

    for (unsigned int i = 0; i < n_messages; ++i)
    {
        fifo->Send("");
    }

    WaitForever();
}

void drivers::LatticeAgreement(Parser &parser) noexcept
{
    auto id = parser.id();
    auto hosts = parser.hosts();
    auto local_host = parser.local_host();
    auto lattice_p = parser.lattice_p();
    auto lattice_vs = parser.lattice_vs();
    auto lattice_ds = parser.lattice_ds();
    auto lattice_proposals = parser.lattice_proposals();

    std::cout << "[INFO] Lattice Agreement Mode Activated\n"
              << "[INFO] ===========================\n"
              << "[INFO] id = " << local_host.id << "\n"
              << "[INFO] ip = " << local_host.ip_readable() << "\n"
              << "[INFO] lattice_p = " << lattice_p << "\n"
              << "[INFO] lattice_vs = " << lattice_vs << "\n"
              << "[INFO] lattice_ds = " << lattice_ds << "\n";

    try
    {
        logger.emplace(parser.output_path(), true);
        server.emplace(local_host.ip, local_host.port);
        client.emplace(server.value().sockfd());
        manager = std::make_unique<::LatticeAgreement>(logger.value(), id);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        std::exit(EXIT_FAILURE);
    }

    auto lattice = dynamic_cast<::LatticeAgreement *>(manager.get());

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
                                                        client.value());
                lattice->Add(std::move(pl));
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
                std::exit(EXIT_FAILURE);
            }
        }
    }

    std::size_t max_receive_size = (lattice_ds * sizeof(unsigned int)) +
                                   PerfectLink::kPacketPrefixSize +
                                   Broadcast::kPacketPrefixSize +
                                   LatticeAgreement::kPacketPrefixSize;

#ifdef DEBUG
    std::cout << "[DBUG] Setting max_receive_size = " << max_receive_size << std::endl;
#endif

    server.value().max_receive_size(max_receive_size);
    server.value().Start();
    lattice->Start();

    for (const auto &proposal : lattice_proposals)
    {
        lattice->Propose(proposal);
    }

    WaitForever();
}