// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>
#include <chrono>

struct GrayscaleImage
{
    std::chrono::system_clock::time_point timestamp;
    int                                   width;
    int                                   height;
    std::vector<uint8_t>                  data;

    GrayscaleImage(int w, int h, std::chrono::system_clock::time_point t);
};
