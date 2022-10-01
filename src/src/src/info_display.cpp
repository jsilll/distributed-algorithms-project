#include "info_display.hpp"

#include <iostream>

void display_host_info(Parser &parser)
{
    std::cout << "[INFO] My ID: " << parser.id() << "\n";
    std::cout << "[INFO] My PID: " << getpid() << "\n";
    std::cout << "[INFO] From a new terminal type `kill -SIGINT "
              << getpid() << "' or 'kill -SIGTERM "
              << getpid() << "' to stop processing packets.\n[INFO]\n";
}

void display_hosts_info(Parser &parser)
{
    std::cout << "[INFO] List of resolved hosts is:\n";
    std::cout << "[INFO] ==========================\n";
    auto hosts = parser.hosts();
    for (auto &host : hosts)
    {
        std::cout << "[INFO] " << host.id << "\n";
        std::cout << "[INFO] Human-readable IP: " << host.ip_readable() << "\n";
        std::cout << "[INFO] Machine-readable IP: " << host.ip << "\n";
        std::cout << "[INFO] Human-readbale Port: " << host.port_readable() << "\n";
        std::cout << "[INFO] Machine-readbale Port: " << host.port << "\n";
        std::cout << "[INFO]\n";
    }
    std::cout << "[INFO]\n";
}

void display_exec_args_info(Parser &parser)
{
    std::cout << "[INFO] Path to hosts:\n";
    std::cout << "[INFO] ===============\n";
    std::cout << "[INFO] " << parser.hosts_path() << "\n[INFO]\n";
    std::cout << "[INFO] Path to output:\n";
    std::cout << "[INFO] ===============\n";
    std::cout << "[INFO] " << parser.output_path() << "\n[INFO]\n";
    if (parser.requires_config())
    {
        std::cout << "[INFO] Path to config:\n";
        std::cout << "[INFO] ===============\n";
        std::cout << "[INFO] " << parser.config_path() << "\n[INFO]\n";
    }
}