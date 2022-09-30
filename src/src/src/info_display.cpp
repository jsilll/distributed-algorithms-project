#include "info_display.hpp"

#include <iostream>

void display_host_info(Parser &parser)
{
    std::cout << "My ID: " << parser.id() << "\n";
    std::cout << "My PID: " << getpid() << "\n";
}

void display_hosts_info(Parser &parser)
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

void display_exec_args_info(Parser &parser)
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