// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMGACQUISITIONAPP_H
#define IMGACQUISITIONAPP_H

#include <memory>

#include <QCoreApplication>
#include <QtGui/QKeyEvent>

#include "CamThread.h"
#include "NvEncGlue.h"
#include "SharedMemory.h"
#include <memory>

// inherits from QCoreApplication
class ImgAcquisitionApp : public QCoreApplication
{
    Q_OBJECT

public:
    /**
     * @brief Initializes acquisition/shared memory threads and runs them.
     *
     * Also runs a watchdog indefinately. DOES NOT RETURN.
     *
     * @param Process argc
     * @param Process argv
     */
    ImgAcquisitionApp(int& argc, char** argv);

    /**
     * @brief STUB
     */
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

    /**
     * @brief Find and fix any partially written videos
     *
     * might print error messages if resolving failed.
     * This might be the case when the textfile was empty.
     */
    void resolveLocks();

private:
    //! Shared memory thread pointer
    beeCompress::SharedMemory* _smthread;

    //! A vector of the class CamThread, they are accessed from the constructor
    std::unique_ptr<CamThread> _threads[4];

    //! Number of detected cameras
    unsigned int _numCameras;

    //! Glue objects which handle encoder workers
    beeCompress::NvEncGlue _glue1, _glue2;

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
