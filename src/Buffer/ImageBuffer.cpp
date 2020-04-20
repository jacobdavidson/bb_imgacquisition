// SPDX-License-Identifier: GPL-3.0-or-later

#include "ImageBuffer.h"

ImageBuffer::ImageBuffer(int w, int h, int cid, std::string t)
: timestamp{t}
, width{w}
, height{h}
, camid{cid}
, data(width * height)
{
}
