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



namespace fmt {

struct base_time_formatter {
    char presentation = 'b'; // Default presentation ('b' for basic, 'e' for extended)
    std::optional<std::size_t> precision;

    // Parses format specifications of the form ['b' | 'e'] ['.' ('1' - '9')].
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && (*it == 'b' || *it == 'e')) {
            presentation = *it++;
        }
        if (it != end && *it == '.') {
            ++it;
            if (it != end && '1' <= *it && *it <= '9') {
                precision = static_cast<std::size_t>(*it++ - '0');
            } else {
                throw format_error("invalid format");
            }
        }
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }

    // Utility for formatting time_point to a string with specified presentation and precision
    template <typename Clock, typename Duration, typename FormatContext>
    auto format_time_point(const std::chrono::time_point<Clock, Duration>& tp, FormatContext& ctx) -> decltype(ctx.out()) {
        std::time_t tt = Clock::to_time_t(tp);
        std::tm* tm = std::gmtime(&tt); // Note: std::gmtime is not thread-safe

        // Handling fractional seconds precision if specified
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count() % 1000000000;
        std::string fractional_seconds = precision ? fmt::format(".{:0{}}", nanos / std::pow(10, 9 - *precision), *precision) : "";

        if (presentation == 'b') {
            return fmt::format_to(ctx.out(), "{:%Y%m%dT%H%M%S}{}Z", *tm, fractional_seconds);
        } else {
            return fmt::format_to(ctx.out(), "{:%Y-%m-%dT%H:%M:%S}{}Z", *tm, fractional_seconds);
        }
    }
};

// Generic formatter
// Should apply to :
// std::chrono::system_clock
// std::chrono::steady_clock
// std::chrono::nanoseconds
// std::chrono::microseconds
// std::chrono::seconds
// but see below for additions to ensure functionality with libfmt v8
template<typename Clock, typename Duration, typename Char>
struct formatter<std::chrono::time_point<Clock, Duration>, Char> : base_time_formatter {
    using time_point = std::chrono::time_point<Clock, Duration>;

    constexpr auto parse(format_parse_context& ctx) {
        return base_time_formatter::parse(ctx);
    }

    template<typename FormatContext>
    auto format(const time_point& tp, FormatContext& ctx) {
        return base_time_formatter::format_time_point(tp, ctx);
    }
};

// Specialization for different types, to ensure they are recognized (added due to problems with libfmt v8)
// system_clock with nanoseconds needed to be treated differently

// Specialization for std::chrono::system_clock with std::chrono::nanoseconds
template<>
struct formatter<std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>, char> : base_time_formatter {
    constexpr auto parse(format_parse_context& ctx) {
        return base_time_formatter::parse(ctx);
    }

    template<typename FormatContext>
    auto format(const std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>& tp, FormatContext& ctx) {
        return base_time_formatter::format_time_point(tp, ctx);
    }
};

// system_clock with microseconds
template<>
struct formatter<std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>, char>
    : public formatter<std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>::clock, 
                       std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>::duration, char> {
    // Inherits everything from the base formatter
};

// system_clock with seconds
template<>
struct formatter<std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>, char>
    : public formatter<std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>::clock, 
                       std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>::duration, char> {
    // Inherits everything from the base formatter
};

// steady_clock with nanoseconds
template<>
struct formatter<std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>, char>
    : public formatter<std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>::clock, 
                       std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>::duration, char> {
    // Inherits everything from the base formatter
};


// steady_clock with microseconds
template<>
struct formatter<std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds>, char>
    : public formatter<std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds>::clock, 
                       std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds>::duration, char> {
    // Inherits everything from the base formatter
};

// steady_clock with seconds
template<>
struct formatter<std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>, char>
    : public formatter<std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>::clock, 
                       std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>::duration, char> {
    // Inherits everything from the base formatter
};

template<typename Char>
struct formatter<std::thread::id, Char>
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
struct formatter<boost::filesystem::path, Char>
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
struct formatter<QtMsgType, Char>
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
struct formatter<QString, Char>
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

} // end using namespace fmt
