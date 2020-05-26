// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include <xiApi.h>

#include "Camera.h"

class XimeaCamera : public Camera
{
    Q_OBJECT

public:
    XimeaCamera(Config config, VideoStream videoStream, Watchdog* watchdog);

private:
    void initCamera();
    void startCapture();

    void enforce(XI_RETURN errorCode, const std::string& operation = "");

    HANDLE _Camera;

protected:
    void run();
};
