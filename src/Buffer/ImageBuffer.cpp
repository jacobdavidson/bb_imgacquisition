#include "ImageBuffer.h"

namespace beeCompress
{
    ImageBuffer::ImageBuffer(int w, int h, int cid, std::string t)
    : timestamp{t}
    , width{w}
    , height{h}
    , camid{cid}
    , data(width * height)
    {
    }
}
