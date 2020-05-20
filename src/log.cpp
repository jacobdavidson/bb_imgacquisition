// SPDX-License-Identifier: GPL-3.0-or-later

#include "log.h"

#include <mutex>

#include <QDebug>

namespace log_detail
{
    static std::recursive_mutex& mutex()
    {
        static std::recursive_mutex instance;
        return instance;
    }

    void Logger::debug(const char* msg) const
    {
        auto lock = std::unique_lock(mutex());
        qLogger.debug() << msg;
    }

    void Logger::info(const char* msg) const
    {
        auto lock = std::unique_lock(mutex());
        qLogger.info() << msg;
    }

    void Logger::warning(const char* msg) const
    {
        auto lock = std::unique_lock(mutex());
        qLogger.warning() << msg;
    }

    void Logger::critical(const char* msg) const
    {
        auto lock = std::unique_lock(mutex());
        qLogger.critical() << msg;
    }
}
