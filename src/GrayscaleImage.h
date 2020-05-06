// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

struct GrayscaleImage
{
    std::string          timestamp;
    int                  width;
    int                  height;
    std::vector<uint8_t> data;

    GrayscaleImage(int w, int h, std::string t);
};
