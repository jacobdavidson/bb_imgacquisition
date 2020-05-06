// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include <QString>
#include <QThread>

#include "VideoStream.h"
#include "settings/Settings.h"

// Forward declare not-required types.
class Watchdog;

/*!\brief Thread object which acquires images from a camera.
 *
 * Contains functions to initialize the cameras and run the acquistion.
 */
class CamThread : public QThread
{
    Q_OBJECT

protected:
    using Config = SettingsIAC::VideoStream::Camera;

    CamThread(Config config, VideoStream videoStream, Watchdog* watchdog);

    Config      _config;
    VideoStream _videoStream;
    Watchdog*   _watchdog;

public:
    virtual ~CamThread();

signals:
    void logMessage(int logLevel, QString message);

public:
    static CamThread* make(Config config, VideoStream videoStream, Watchdog* watchdog);
};
