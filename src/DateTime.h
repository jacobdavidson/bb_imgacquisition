// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <ctime>
#include <chrono>

// Immutable calendar date and time in UTC
struct DateTime final
{
public:
    DateTime() = delete;

    DateTime(std::tm tm, std::chrono::microseconds::rep us);

    const std::tm                        tm;
    const std::chrono::microseconds::rep us;

    // Return current DateTime according to system clock
    static DateTime now();
};
