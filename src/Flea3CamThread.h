// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "CamThread.h"
#include <FlyCapture2.h>

#include "Watchdog.h"
#include <mutex>

/*!\brief Thread object which acquires images from a camera.
 *
 * Inherits from Qthread for threading.
 * Contains functions to initialize the cameras and run the acquistion.
 */
class Flea3CamThread : public CamThread
{
    Q_OBJECT

public:
    Flea3CamThread(Config config, VideoStream videoStream, Watchdog* watchdog);

private:
    bool initCamera();
    bool startCapture();

    //! @brief Just prints the camera's info
    void PrintCameraInfo(FlyCapture2::CameraInfo* pCamInfo);

    //! @brief Prints the camera's format capabilities
    void PrintFormat7Capabilities(
        FlyCapture2::Format7Info
            fmt7Info); // We will use Format7 to set the video parameters instead of DCAM,
                       // so it becomes handy to print this info

    /**
     * @brief Checks whether the error code is OK and logs otherwise
     *
     * @param The error object
     */
    bool checkReturnCode(FlyCapture2::Error error);

    /**
     * @brief Sends an error message.
     *
     * Currently all of them get logged to console.
     *
     * @param The log level (currently ugnored)
     * @param Message forwarded to QDebug()
     */
    void sendLogMessage(int logLevel, QString message);

    /**
     * @brief Generates a log message to log.txt in the given path.
     *
     * @param Path to the log.txt file
     * @param Message to emit
     */
    void generateLog(QString path, QString message);

    /**
     * @brief Logs a critical error in very detail
     *
     * Logs are written to the logfile specified as per
     * JSON configuration.
     *
     * @param The error object
     */
    void logCriticalError(FlyCapture2::Error e);

    //! Deprecated JPeg compression parameter
    // JPEGOption            _jpegConf;

    //! objects that points to the camera
    FlyCapture2::Camera _Camera;

    //! object to catch the image temporally
    FlyCapture2::Image _Image;

    //! object needed to read the frame counter
    FlyCapture2::ImageMetadata _ImInfo;

    //! the current frame in Image
    unsigned int _FrameNumber;

    //! time stamp from the current frame
    FlyCapture2::TimeStamp _TimeStamp;

    //! ... to enumerate each image in a second
    unsigned int _LocalCounter;

protected:
    void run(); // this is the function that will be iterated indefinitely
};
