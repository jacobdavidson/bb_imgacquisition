// SPDX-License-Identifier: GPL-3.0-or-later

#include "GrayscaleImage.h"

GrayscaleImage::GrayscaleImage(int w, int h, std::string t)
: timestamp{t}
, width{w}
, height{h}
, data(width * height)
{
}
