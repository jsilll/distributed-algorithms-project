#pragma once

#include "parser.hpp"
#include "logger.hpp"

namespace drivers
{
    /**
     * @brief Responsible for executing
     * the executing the perfect links algorithm
     *
     */
    void PerfectLinks(unsigned long int id,
                      unsigned long receiver_id,
                      unsigned long n_messages,
                      const std::vector<Parser::Host> &hosts,
                      Logger &logger);

} // namespace drivers
