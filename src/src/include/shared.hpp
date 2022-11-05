#pragma once

#include <mutex>

template <typename T>
struct Shared
{
    std::shared_mutex mutex;
    T data;

    Shared() noexcept = default;
};