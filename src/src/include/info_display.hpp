#pragma once

#include "parser.hpp"

/**
 * @brief Displays the process's information
 *
 * @param parser
 */
void display_host_info(Parser &parser);

/**
 * @brief Displays the information about
 * the other processes' on the network
 *
 * @param parser
 */
void display_hosts_info(Parser &parser);

/**
 * @brief Displays information about the
 * execution arguments
 *
 * @param parser
 */
void display_exec_args_info(Parser &parser);