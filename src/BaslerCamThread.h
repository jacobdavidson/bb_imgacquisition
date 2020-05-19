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
    void initCamera();
    void startCapture();

    // Pylon camera object
    Pylon::CBaslerUsbInstantCamera _camera;
    // Pylon camera capture result
    Pylon::CGrabResultPtr _grabbed;

protected:
    void run(); // this is the function that will be iterated indefinitely
};
