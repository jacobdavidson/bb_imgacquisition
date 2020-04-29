// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <unordered_map>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/frame.h>
}

#include "GrayscaleImage.h"

class VideoWriter final
{
public:
    struct Config final
    {
        int width;
        int height;

        struct
        {
            int num;
            int den;
        } framerate;

        struct
        {
            std::string                                  name;
            std::unordered_map<std::string, std::string> options;
        } codec;
    };

    VideoWriter(const std::string& filename, Config config);
    ~VideoWriter();

    void write(const GrayscaleImage& image);
    void close();

private:
    VideoWriter(Config config);

    Config _cfg;

    AVFormatContext* _formatContext;

    AVStream* _videoStream;

    AVCodecContext* _codecContext;

    AVFrame*  _videoFrame;
    AVPacket* _videoPacket;

    int64_t _videoFrameIndex;
};
