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

static void customMessageHandler(QtMsgType                 type,
                                 const QMessageLogContext& context,
                                 const QString&            msg);

static const QtMessageHandler g_defaultMessageHandler = []() {
    qSetMessagePattern("%{if-category}%{category}: %{endif}%{type}:\t%{message}");
    return qInstallMessageHandler(customMessageHandler);
}();

static void customMessageHandler(QtMsgType                 type,
                                 const QMessageLogContext& context,
                                 const QString&            msg)
{
    auto lock = std::unique_lock(log_detail::mutex());
    (*g_defaultMessageHandler)(type, context, msg);
}
