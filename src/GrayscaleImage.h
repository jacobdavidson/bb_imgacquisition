// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>

#include <QVector>

struct GrayscaleImage
{
    std::chrono::system_clock::time_point timestamp;
    int                                   width;
    int                                   height;
    QVector<uint8_t>                      data;

    GrayscaleImage() = default;
    GrayscaleImage(int w, int h, std::chrono::system_clock::time_point t);
};
