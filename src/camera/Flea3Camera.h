// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <FlyCapture2.h>

#include "Camera.h"

/*!\brief Thread object which acquires images from a camera.
 *
 * Inherits from Qthread for threading.
 * Contains functions to initialize the cameras and run the acquistion.
 */
class Flea3Camera : public Camera
{
    Q_OBJECT

public:
    Flea3Camera(Config config, VideoStream videoStream, Watchdog* watchdog);

private:
    void initCamera();
    void startCapture();

    //! @brief Just prints the camera's info
    void PrintCameraInfo(FlyCapture2::CameraInfo* pCamInfo);

    //! @brief Prints the camera's format capabilities
    // We will use Format7 to set the video parameters instead of DCAM,
    // so it becomes handy to print this info
    void PrintFormat7Capabilities(FlyCapture2::Format7Info fmt7Info);

    void enforce(FlyCapture2::Error error);

    //! objects that points to the camera
    FlyCapture2::Camera _Camera;

    //! object to catch the image temporally
    FlyCapture2::Image _Image;

protected:
    void run(); // this is the function that will be iterated indefinitely
};
