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

class VideoFileWriter final
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

    VideoFileWriter(const std::string& filename, Config config);
    ~VideoFileWriter();

    void write(const GrayscaleImage& image);
    void close();

private:
    VideoFileWriter(Config config);

    void encodeFrame(AVFrame* frame);

    Config _cfg;

    std::string _filename;

    AVFormatContext* _formatContext;

    AVStream* _videoStream;

    AVCodecContext* _codecContext;

    AVFrame*  _videoFrame;
    AVPacket* _videoPacket;

    std::int64_t _videoFrameIndex;
};
