// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mutex>
#include <string>
#include <optional>

#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCamera.h>

#include "CamThread.h"
#include "Watchdog.h"

/*!\brief Thread object which acquires images from a camera.
 *
 * Contains functions to initialize the cameras and run the acquistion.
 */
class BaslerCamThread : public CamThread
{
    Q_OBJECT

private:
    Pylon::PylonAutoInitTerm pylon;

public:
    BaslerCamThread(Config config, VideoStream videoStream, Watchdog* watchdog);

private:
    bool initCamera();
    bool startCapture();

    //! @brief Just prints the camera's info
    // void                PrintCameraInfo(CameraInfo *pCamInfo);

    /**
     * @brief Checks whether the error code is OK and logs otherwise
     *
     * @param Ximea error code
     */
    bool checkReturnCode(int errorCode, const std::string& operation = "");

    /**
     * @brief Sends an error message.
     *
     * Currently all of them get logged to console.
     *
     * @param The log level (currently ugnored)
     * @param Message forwarded to QDebug()
     */
    void sendLogMessage(int logLevel, QString message);
    void sendLogMessage(int logLevel, std::string message);
    void sendLogMessage(int logLevel, const char* message);

    /**
     * @brief Generates a log message to log.txt in the given path.
     *
     * @param Path to the log.txt file
     * @param Message to emit
     */
    void generateLog(QString path, QString message);

    /**
     * @brief Deprecated
     *
     * @param Deprecated
     * @param Deprecated
     */
    void localCounter(int oldTime, int newTime);

    /**
     * @brief Logs a critical error in very detail
     *
     * Logs are written to the logfile specified as per
     * JSON configuration.
     *
     * @param The error message
     */
    void logCriticalError(const std::string& message);

    // Pylon camera object
    Pylon::CBaslerUsbInstantCamera _camera;
    // Pylon camera capture result
    Pylon::CGrabResultPtr _grabbed;

    //! ... to enumerate each image in a second
    unsigned int _LocalCounter;

protected:
    void run(); // this is the function that will be iterated indefinitely
};
