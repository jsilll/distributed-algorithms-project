#pragma once

#include "parser.hpp"
#include "logger.hpp"

/**
 * @brief Responsible for executing
 * the executing the perfect links algorithm
 *
 */
void perfect_links_driver(unsigned long int id,
                          unsigned long receiver_id,
                          unsigned long n_messages,
                          const std::vector<Parser::Host> &hosts,
                          Logger &logger);
