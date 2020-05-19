// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef WINDOWS
    #include <windows.h>
#endif
#include "CamThread.h"
#include <xiApi.h>

#include "Watchdog.h"
#include <mutex>
#include <string>

/*!\brief Thread object which acquires images from a camera.
 *
 * Contains functions to initialize the cameras and run the acquistion.
 */
class XimeaCamThread : public CamThread
{
    Q_OBJECT

public:
    XimeaCamThread(Config config, VideoStream videoStream, Watchdog* watchdog);

    //! Serial number of the camera
    std::string _Serial;

private:
    void initCamera();
    void startCapture();

    //! @brief Just prints the camera's info
    // void                PrintCameraInfo(CameraInfo *pCamInfo);

    void enforce(XI_RETURN errorCode, const std::string& operation = "");

    //! Deprecated JPeg compression parameter
    // JPEGOption            _jpegConf;

    //! Handle returned by xiOpenDevice.
    HANDLE _Camera;

protected:
    void run(); // this is the function that will be iterated indefinitely
};
