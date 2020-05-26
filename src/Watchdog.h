// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mutex>
#include <vector>
#include <chrono>

class Watchdog final
{
private:
    std::mutex                                                      _mutex;
    std::vector<std::string>                                        _ids;
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> _lastPulses;

public:
    std::size_t watch(std::string id);
    void        pulse(std::size_t index);
    void        check();
};
