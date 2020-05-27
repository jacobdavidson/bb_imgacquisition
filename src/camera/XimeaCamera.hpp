// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>

#include <xiApi.h>

#include "Camera.hpp"

class XimeaCamera : public Camera
{
    Q_OBJECT

public:
    XimeaCamera(Config config, ImageStream imageStream);

private:
    void initCamera();
    void startCapture();

    void enforce(XI_RETURN errorCode, const std::string& operation = "");

    HANDLE _Camera;

protected:
    void run();
};
