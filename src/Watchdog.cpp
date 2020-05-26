#include "Watchdog.h"

#include "util/format.h"

std::size_t Watchdog::watch(std::string id)
{
    auto       lock  = std::lock_guard(_mutex);
    const auto index = _ids.size();
    _ids.push_back(id);
    _lastPulses.push_back({});
    return index;
}

void Watchdog::pulse(std::size_t index)
{
    auto lock          = std::lock_guard(_mutex);
    _lastPulses[index] = std::chrono::steady_clock::now();
}

void Watchdog::check()
{
    using namespace std::chrono_literals;

    auto       lock = std::lock_guard(_mutex);
    const auto now  = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < _lastPulses.size(); ++i)
    {
        if (now - _lastPulses[i] > 60s)
        {
            throw std::runtime_error(fmt::format("Watchdog timeout for {}", _ids[i]));
        }
    }
}
