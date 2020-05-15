// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <fmt/format.h>
#include <fmt/chrono.h>

template<typename Clock, typename Duration, typename Char>
struct fmt::formatter<std::chrono::time_point<Clock, Duration>, Char>
{
    using time_point = std::chrono::time_point<Clock, Duration>;

    // Presentation format: 'b' - basic, 'e' - extended.
    char presentation = 'b';

    // Fractional second precision
    std::optional<size_t> precision;

    // Parses format specifications of the form ['f' | 'e'].
    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        auto       it  = ctx.begin();
        const auto end = ctx.end();

        if (it != end && (*it == 'b' || *it == 'e'))
        {
            presentation = *it++;
        }
        if (it != end && *it == '.')
        {
            ++it;
            if (it != end && '1' <= *it && *it <= '9')
            {
                precision = static_cast<unsigned int>(*it++ - '0');
            }
            else
            {
                throw format_error("invalid format");
            }
        }

        if (it != end && *it != '}')
        {
            throw format_error("invalid format");
        }

        return it;
    }

    template<typename FormatContext>
    auto format(const time_point& tp, FormatContext& ctx)
    {
        const auto tm = fmt::gmtime(Clock::to_time_t(tp));

        if (precision)
        {
            const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                tp.time_since_epoch())
                                .count() %
                            1000000000;
            const decltype(ns) tr = std::trunc(static_cast<double>(ns) /
                                               (std::pow(10, 9 - *precision)));

            if (presentation == 'b')
            {
                return format_to(ctx.out(), "{:%Y%m%dT%H%M%S}.{:0{}}Z", tm, tr, *precision);
            }
            else if (presentation == 'e')
            {
                return format_to(ctx.out(), "{:%Y-%m-%dT%H:%M:%S}.{:0{}}Z", tm, tr, *precision);
            }
        }
        else
        {
            if (presentation == 'b')
            {
                return format_to(ctx.out(), "{:%Y%m%dT%H%M%S}Z", tm);
            }
            else if (presentation == 'e')
            {
                return format_to(ctx.out(), "{:%Y-%m-%dT%H:%M:%S}Z", tm);
            }

            assert(false);
        }

        assert(false);
    }
};
