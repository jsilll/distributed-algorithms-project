#pragma once

#include <mutex>

/**
 * @brief Generic struct for storing an assocatied shared_mutex
 * 
 * @tparam T 
 */
template <typename T>
struct Shared
{
    std::shared_mutex mutex;
    T data;

    Shared() = default;
};