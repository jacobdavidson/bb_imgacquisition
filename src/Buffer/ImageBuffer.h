#pragma once

#include <string>
#include <vector>

namespace beeCompress
{
    struct ImageBuffer
    {
        std::string timestamp;
        int         width;
        int         height;
        int         camid;
        std::vector<uint8_t> data;

        ImageBuffer(int w, int h, int cid, std::string t);
    };
}
