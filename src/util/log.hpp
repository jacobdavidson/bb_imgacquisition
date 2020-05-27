// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMessageLogger>

#include "format.hpp"

namespace log_detail
{
    class Logger final
    {
    public:
        constexpr Logger()
        : qLogger()
        {
        }
        constexpr Logger(const char* file, int line, const char* function)
        : qLogger(file, line, function, "default")
        {
        }
        constexpr Logger(const char* file, int line, const char* function, const char* category)
        : qLogger(file, line, function, category)
        {
        }

        Logger(const Logger&)     = delete;
        Logger(Logger&&) noexcept = delete;
        Logger& operator=(const Logger&) = delete;
        Logger& operator=(Logger&&) noexcept = delete;

    public:
        void              debug(const char* msg) const;
        void              info(const char* msg) const;
        void              warning(const char* msg) const;
        void              critical(const char* msg) const;
        [[noreturn]] void fatal(const char* msg) const;

        template<typename Str, typename... Args>
        void debug(const Str& fmt, Args&&... args) const
        {
            auto msg = fmt::format(fmt, std::forward<Args>(args)...);
            debug(msg.c_str());
        }

        template<typename Str, typename... Args>
        void info(const Str& fmt, Args&&... args) const
        {
            auto msg = fmt::format(fmt, std::forward<Args>(args)...);
            info(msg.c_str());
        }

        template<typename Str, typename... Args>
        void warning(const Str& fmt, Args&&... args) const
        {
            auto msg = fmt::format(fmt, std::forward<Args>(args)...);
            warning(msg.c_str());
        }

        template<typename Str, typename... Args>
        void critical(const Str& fmt, Args&&... args) const
        {
            auto msg = fmt::format(fmt, std::forward<Args>(args)...);
            critical(msg.c_str());
        }

    private:
        QMessageLogger qLogger;
    };
}

#define logDebug                                                                                  \
    log_detail::Logger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).debug
#define logInfo log_detail::Logger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).info
#define logWarning                                                                                \
    log_detail::Logger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).warning
#define logCritical                                                                               \
    log_detail::Logger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).critical
