#include "Watchdog.h"

void Watchdog::pulse()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _lastPulses[std::this_thread::get_id()] = std::chrono::steady_clock::now();
}

void Watchdog::check()
{
    std::lock_guard<std::mutex> lock(_mutex);
    const auto                  now = std::chrono::steady_clock::now();
    for (const auto& [threadId, lastPulse] : _lastPulses)
    {
        if (now - lastPulse > std::chrono::seconds(60))
        {
            std::cerr << "Watchdog timeout for thread " << threadId;
            std::exit(1);
        }
    }
}
