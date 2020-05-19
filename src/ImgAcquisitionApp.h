// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include <QCoreApplication>
#include <QtGui/QKeyEvent>

#include "CamThread.h"
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

    /**
     * @brief prints FlyCapture2 and Application build info
     *
     * Just prints the library's info
     */
    void printBuildInfo();

    /**
     * @brief This function checks that at least one camera is connected
     */
    int checkCameras();

private:
    Watchdog _watchdog;

    //! A vector of the class CamThread, they are accessed from the constructor
    std::vector<std::unique_ptr<CamThread>> _cameraThreads;

    //! Number of detected cameras
    unsigned int _numCameras;

    std::unordered_map<std::string, VideoWriteThread> _videoWriterThreads;
};
