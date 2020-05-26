// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include <QCoreApplication>

#include "camera/Camera.h"
#include "ImageStreamsWriter.h"
#include "Watchdog.h"

class ImgAcquisitionApp : public QCoreApplication
{
    Q_OBJECT

public:
    ImgAcquisitionApp(int& argc, char** argv);
    ~ImgAcquisitionApp();

private:
    Watchdog _watchdog;

    std::vector<std::unique_ptr<Camera>> _cameraThreads;

    std::unordered_map<std::string, ImageStreamsWriter> _imageStreamsWriters;
};
