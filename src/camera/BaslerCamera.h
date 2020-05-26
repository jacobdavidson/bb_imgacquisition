// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mutex>
#include <optional>
#include <chrono>
#include <vector>

#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCamera.h>

#include "Camera.h"

class BaslerCamera : public Camera
{
    Q_OBJECT

private:
    Pylon::PylonAutoInitTerm pylon;

public:
    BaslerCamera(Config config, VideoStream videoStream);

    static std::vector<Config> getAvailable();

private:
    void initCamera();
    void startCapture();

    Pylon::CBaslerUsbInstantCamera _camera;

    std::chrono::nanoseconds _nsPerTick;

    Pylon::CGrabResultPtr _grabbed;

protected:
    void run();
};
