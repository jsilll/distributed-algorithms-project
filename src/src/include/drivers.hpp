#pragma once

#include "parser.hpp"

/**
 * @brief Responsible for executing
 * the executing the perfect links algorithm
 *
 */
void perfect_links_driver(unsigned long n_messages, unsigned long receiver_id, Parser::Host localhost);