// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mutex>
#include <thread>
#include <unordered_map>
#include <chrono>

class Watchdog final
{
private:
    std::mutex _mutex;
    std::unordered_map<std::thread::id, std::chrono::time_point<std::chrono::steady_clock>>
        _lastPulses;

public:
    void pulse();
    void check();
};
