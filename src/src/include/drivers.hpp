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
     * @brief Responsible for stopping
     * all the threads (if there any running)
     *
     */
    void StopExecution() noexcept;

    /**
     * @brief Responsible for executing
     * the executing the perfect links algorithm
     *
     */
    void PerfectLinks(Parser &parser) noexcept;

    /**
     * @brief Responsible for executing
     * the executing the perfect links algorithm
     *
     */
    void FIFOBroadcast(Parser &parser) noexcept;
} // namespace drivers
