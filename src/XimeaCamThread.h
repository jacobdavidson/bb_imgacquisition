// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include <xiApi.h>

#include "CamThread.h"

/*!\brief Thread object which acquires images from a camera.
 *
 * Contains functions to initialize the cameras and run the acquistion.
 */
class XimeaCamThread : public CamThread
{
    Q_OBJECT

public:
    XimeaCamThread(Config config, VideoStream videoStream, Watchdog* watchdog);

private:
    void initCamera();
    void startCapture();

    //! @brief Just prints the camera's info
    // void                PrintCameraInfo(CameraInfo *pCamInfo);

    void enforce(XI_RETURN errorCode, const std::string& operation = "");

    //! Handle returned by xiOpenDevice.
    HANDLE _Camera;

protected:
    void run(); // this is the function that will be iterated indefinitely
};
