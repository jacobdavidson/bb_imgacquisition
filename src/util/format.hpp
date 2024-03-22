// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cassert>
#include <thread>
#include <sstream>
#include <iomanip>

#include <QtGlobal>
#include <QString>

#include <boost/filesystem/path.hpp>

#include <fmt/format.h>
#include <fmt/chrono.h>
#include <optional>
#include <cmath>

template<typename Clock, typename Duration, typename Char>
struct fmt::formatter<std::chrono::time_point<Clock, Duration>, Char>
{
    using time_point = std::chrono::time_point<Clock, Duration>;

    // Presentation format: 'b' - basic, 'e' - extended.
    char presentation = 'b';

    // Fractional second precision
    std::optional<std::size_t> precision;

    // Parses format specifications of the form ['b' | 'e'] ['.' ('1' - '9')].
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
        // NOTE: std::gmtime is not thread safe, fmt::gmtime is
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
        }

        throw std::logic_error("");
    }
};

template<typename Char>
struct fmt::formatter<std::thread::id, Char>
{
    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        auto       it  = ctx.begin();
        const auto end = ctx.end();

        if (it != end && *it != '}')
        {
            throw format_error("invalid format");
        }

        return it;
    }

    template<typename FormatContext>
    auto format(const std::thread::id& id, FormatContext& ctx)
    {
        std::stringstream ss;
        ss << id;
        return format_to(ctx.out(), "{}", ss.str());
    }
};

template<typename Char>
struct fmt::formatter<boost::filesystem::path, Char>
{
    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        auto       it  = ctx.begin();
        const auto end = ctx.end();

        if (it != end && *it != '}')
        {
            throw format_error("invalid format");
        }

        return it;
    }

    template<typename FormatContext>
    auto format(const boost::filesystem::path& path, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", path.string());
    }
};

template<typename Char>
struct fmt::formatter<QtMsgType, Char>
{
    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        auto       it  = ctx.begin();
        const auto end = ctx.end();

        if (it != end && *it != '}')
        {
            throw format_error("invalid format");
        }

        return it;
    }

    template<typename FormatContext>
    auto format(const QtMsgType& msgType, FormatContext& ctx)
    {
        switch (msgType)
        {
        case QtDebugMsg:
            return format_to(ctx.out(), "debug");
        case QtWarningMsg:
            return format_to(ctx.out(), "warning");
        case QtCriticalMsg:
            return format_to(ctx.out(), "critical");
        case QtFatalMsg:
            return format_to(ctx.out(), "fatal");
        case QtInfoMsg:
            return format_to(ctx.out(), "info");
        default:
            return format_to(ctx.out(), "unknown");
        }
    }
};

template<typename Char>
struct fmt::formatter<QString, Char>
{
    bool quoted = false;

    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        auto       it  = ctx.begin();
        const auto end = ctx.end();

        if (it != end && (*it == 'q'))
        {
            quoted = true;
            ++it;
        }

        if (it != end && *it != '}')
        {
            throw format_error("invalid format");
        }

        return it;
    }

    template<typename FormatContext>
    auto format(const QString& s, FormatContext& ctx)
    {
        if (quoted)
        {
            std::stringstream ss;
            ss << std::quoted(s.toStdString());
            return format_to(ctx.out(), ss.str());
        }
        else
        {
            return format_to(ctx.out(), s.toStdString());
        }
    }
};
