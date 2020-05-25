// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mutex>
#include <string>
#include <optional>
#include <chrono>

#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCamera.h>

#include "Camera.h"
#include "Watchdog.h"

/*!\brief Thread object which acquires images from a camera.
 *
 * Contains functions to initialize the cameras and run the acquistion.
 */
class BaslerCamera : public Camera
{
    Q_OBJECT

private:
    Pylon::PylonAutoInitTerm pylon;

public:
    BaslerCamera(Config config, VideoStream videoStream, Watchdog* watchdog);

private:
    void initCamera();
    void startCapture();

    // Pylon camera object
    Pylon::CBaslerUsbInstantCamera _camera;
    //! Timestamp Tick Frequency converted to nanoseconds
    std::chrono::nanoseconds _nsPerTick;
    // Pylon camera capture result
    Pylon::CGrabResultPtr _grabbed;

protected:
    void run(); // this is the function that will be iterated indefinitely
};
