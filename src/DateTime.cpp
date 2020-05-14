// SPDX-License-Identifier: GPL-3.0-or-later
#include "DateTime.h"

#include <fmt/format.h>
#if FMT_VERSION >= 60000
    #include <fmt/chrono.h>
#else
    #include <fmt/time.h>
#endif

DateTime::DateTime(std::tm _tm, std::chrono::microseconds::rep _us)
: tm{_tm}
, us{_us}
{
}

DateTime DateTime::now()
{
    const auto now = std::chrono::system_clock::now();
    const auto now_us =
        std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() %
        1000000;

    return {fmt::gmtime(std::chrono::system_clock::to_time_t(now)), now_us};
}
