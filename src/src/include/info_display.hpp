#pragma once

#include "parser.hpp"

namespace info_display
{
    /**
     * @brief Displays the process's information
     *
     * @param parser
     */
    void Localhost(Parser &parser);

    /**
     * @brief Displays the information about
     * the other processes' on the network
     *
     * @param parser
     */
    void Hosts(Parser &parser);

    /**
     * @brief Displays information about the
     * execution arguments
     *
     * @param parser
     */
    void ExecArgs(Parser &parser);

} // namespace info_display
