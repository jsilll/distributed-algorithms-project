#pragma once

#include <optional>

#include "parser.hpp"
#include "logger.hpp"
#include "udp_server.hpp"
#include "udp_client.hpp"
#include "perfect_link.hpp"

namespace drivers
{
    /**
     * @brief Responsible for executing
     * the executing the perfect links algorithm
     *
     */
    void PerfectLinks(Parser &parser,
                      Logger &logger,
                      UDPServer &server,
                      UDPClient &client,
                      std::vector<std::unique_ptr<PerfectLink>> &perect_links);

} // namespace drivers
