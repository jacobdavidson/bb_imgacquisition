// SPDX-License-Identifier: GPL-3.0-or-later

#include "VideoWriter.h"

#include <array>
#include <sstream>

VideoWriter::VideoWriter()
: _formatContext(nullptr)
, _videoStream(nullptr)
, _codecContext(nullptr)
{
}

VideoWriter::~VideoWriter()
{
    if (_codecContext)
    {
        avcodec_free_context(&_codecContext);
    }

    if (_formatContext)
    {
        avformat_free_context(_formatContext);
    }
}

static const char* av_strerror(int errnum)
{
    static thread_local std::array<char, AV_ERROR_MAX_STRING_SIZE> errstr;
    av_make_error_string(&errstr[0], errstr.size(), errnum);
    return &errstr[0];
}

static constexpr auto g_preferred_software_encoder = "libx265";

VideoWriter::VideoWriter(const std::string& filename)
: VideoWriter()
{
    // Set up for encoding a single video stream and storing it in filename with
    // the container format automatically detected based on the filename suffix.

    if (auto r = avformat_alloc_output_context2(&_formatContext, NULL, NULL, filename.c_str());
        r < 0)
        throw std::runtime_error(av_strerror(r));

    auto codec = avcodec_find_encoder_by_name(g_preferred_software_encoder);
    if (!codec)
    {
        std::ostringstream msg;
        msg << "Could not find video encoder: " << g_preferred_software_encoder;
        throw std::runtime_error(msg.str());
    }

    if (!avformat_query_codec(_formatContext->oformat, codec->id, FF_COMPLIANCE_VERY_STRICT))
    {
        std::ostringstream msg;
        msg << "Cannot store " << codec->name << " encoded video in an "
            << _formatContext->oformat->name << " container";
        throw std::runtime_error(msg.str());
    }

    if (auto r = avio_open2(&_formatContext->pb, filename.c_str(), AVIO_FLAG_WRITE, NULL, NULL);
        r < 0)
    {
        std::ostringstream msg;
        msg << "Could not open " << filename << " for writing: " << av_strerror(r);
        throw std::runtime_error(msg.str());
    }

    if (!(_videoStream = avformat_new_stream(_formatContext, codec)))
    {
        throw std::runtime_error("Could not create new video stream");
    }

    if (!(_codecContext = avcodec_alloc_context3(codec)))
    {
        throw std::runtime_error("Could not allocate video encoder state");
    }

    // Configure encoding

    // _codecContext->width = 128;
    // _codecContext->height = 128;

    // _codecContext->framerate = {25, 1};
    // _codecContext->time_base = {1, 25};

    // _codecContext->gop_size = 10;
    // _codecContext->max_b_frames = 1;
    // _codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    // if (codec->id == AV_CODEC_ID_H264) {
    // 	av_opt_set(_codecContext->priv_data, "preset", "slow", 0);
    // }

    if (_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
    {
        _codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    _videoStream->time_base      = _codecContext->time_base;
    _videoStream->avg_frame_rate = _codecContext->framerate;
    _videoStream->r_frame_rate   = _codecContext->framerate;

    if (auto r = avcodec_open2(_codecContext, codec, NULL); r < 0)
    {
        std::ostringstream msg;
        msg << "Could not initialize video encoder: " << av_strerror(r);
        throw std::runtime_error(msg.str());
    }

    if (auto r = avcodec_parameters_from_context(_videoStream->codecpar, _codecContext); r < 0)
    {
        std::ostringstream msg;
        msg << "Could not copy video encoder settings to video stream: " << av_strerror(r);
        throw std::runtime_error(msg.str());
    }
}
