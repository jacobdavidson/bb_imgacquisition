// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/frame.h>
}

class VideoWriter final
{
public:
    VideoWriter(const std::string& filename);

private:
    VideoWriter();
    ~VideoWriter();

    AVFormatContext* _formatContext;

    AVStream* _videoStream;

    AVCodecContext* _codecContext;
};
