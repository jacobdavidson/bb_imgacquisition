// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMGACQUISITIONAPP_H
#define IMGACQUISITIONAPP_H

#include <memory>

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

    /**
     * @brief Find and fix any partially written videos
     *
     * might print error messages if resolving failed.
     * This might be the case when the textfile was empty.
     */
    void resolveLocks();

private:
    Watchdog _watchdog;

    //! A vector of the class CamThread, they are accessed from the constructor
    std::unique_ptr<CamThread> _cameraThreads[4];

    //! Number of detected cameras
    unsigned int _numCameras;

    //! Glue objects which handle encoder workers
    VideoWriteThread _videoWriteThread1;
    VideoWriteThread _videoWriteThread2;

    /**
     * @brief Helper function of resolveLocks
     */
    std::string figureBasename(std::string infile);

    /**
     * @brief Helper function of resolveLocks
     */
    void resolveLockDir(std::string from, std::string to);

    // Slots for the signals sent by CamThread Class
public slots:

    void logMessage(int logLevel, QString message);
};

#endif // IMGACQUISITIONAPP_H
