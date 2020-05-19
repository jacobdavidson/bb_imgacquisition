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
    void initCamera();
    void startCapture();

    //! @brief Just prints the camera's info
    void PrintCameraInfo(FlyCapture2::CameraInfo* pCamInfo);

    //! @brief Prints the camera's format capabilities
    void PrintFormat7Capabilities(
        FlyCapture2::Format7Info
            fmt7Info); // We will use Format7 to set the video parameters instead of DCAM,
                       // so it becomes handy to print this info

    void enforce(FlyCapture2::Error error);

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

protected:
    void run(); // this is the function that will be iterated indefinitely
};
