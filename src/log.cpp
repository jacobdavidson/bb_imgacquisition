// SPDX-License-Identifier: GPL-3.0-or-later

#include "log.h"

#include <mutex>
#include <chrono>
#include <fstream>

#include <QDebug>
#include <QStandardPaths>
#include <QDir>

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

    {
        const auto timestamp = std::chrono::system_clock::now();

        const auto dataLocation = QDir(
            QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
        if (!dataLocation.exists())
        {
            dataLocation.mkpath(".");
        }

        try
        {
            const auto formattedMsg = fmt::format("{:e.3},{},{:q}\n", timestamp, type, msg);

            auto logFile = std::fstream(dataLocation.absoluteFilePath("log.csv").toStdString(),
                                        std::ios::out | std::ios::app);
            logFile.write(formattedMsg.c_str(), formattedMsg.size());
            logFile.close();
        }
        catch (const std::exception& e)
        {
            (*g_defaultMessageHandler)(QtWarningMsg,
                                       QMessageLogContext{QT_MESSAGELOG_FILE,
                                                          QT_MESSAGELOG_LINE,
                                                          QT_MESSAGELOG_FUNC,
                                                          "default"},
                                       e.what());
        }
    }

    (*g_defaultMessageHandler)(type, context, msg);
}
