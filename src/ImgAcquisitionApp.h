// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include <QCoreApplication>

#include "camera/Camera.h"
#include "VideoWriteThread.h"
#include "Watchdog.h"

// inherits from QCoreApplication
class ImgAcquisitionApp : public QCoreApplication
{
    Q_OBJECT

public:
    /**
     * @brief Initializes acquisition threads and runs them.
     *
     * Periodically runs a watchdog monitoring thread heartbeats.
     *
     * @param Process argc
     * @param Process argv
     */
    ImgAcquisitionApp(int& argc, char** argv);
    ~ImgAcquisitionApp();

private:
    Watchdog _watchdog;

    //! A vector of the class Camera, they are accessed from the constructor
    std::vector<std::unique_ptr<Camera>> _cameraThreads;

    //! Number of detected cameras
    unsigned int _numCameras;

    std::unordered_map<std::string, VideoWriteThread> _videoWriterThreads;
};
