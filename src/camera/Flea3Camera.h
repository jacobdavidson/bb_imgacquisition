// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <FlyCapture2.h>

#include "Camera.h"

class Flea3Camera : public Camera
{
    Q_OBJECT

public:
    Flea3Camera(Config config, ImageStream imageStream);

private:
    void initCamera();
    void startCapture();

    void PrintCameraInfo(FlyCapture2::CameraInfo* pCamInfo);

    void PrintFormat7Capabilities(FlyCapture2::Format7Info fmt7Info);

    void enforce(FlyCapture2::Error error);

    FlyCapture2::Camera _Camera;

    FlyCapture2::Image _Image;

protected:
    void run();
};
