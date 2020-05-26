// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include <QString>
#include <QThread>

#include "VideoStream.h"
#include "Settings.h"

// Forward declare not-required types.
class Watchdog;

/*!\brief Thread object which acquires images from a camera.
 *
 * Contains functions to initialize the cameras and run the acquistion.
 */
class Camera : public QThread
{
    Q_OBJECT

protected:
    using Config = Settings::VideoStream::Camera;

    Camera(Config config, VideoStream videoStream, Watchdog* watchdog);

    Config      _config;
    VideoStream _videoStream;
    Watchdog*   _watchdog;

public:
    virtual ~Camera();

    static Camera* make(Config config, VideoStream videoStream, Watchdog* watchdog);

signals:
    void imageCaptured(QString videoStreamId, GrayscaleImage image);
};
