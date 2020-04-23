// SPDX-License-Identifier: GPL-3.0-or-later

#include "VideoWriter.h"

#include <array>
#include <sstream>
#include <cstring>

#if LIBAVFORMAT_VERSION_MAJOR < 58
struct FFmpegInitializer final
{
    FFmpegInitializer()
    {
        av_register_all();
    }
};
static FFmpegInitializer initFFmpeg;
#endif

static const char* av_strerror(int errnum)
{
    static thread_local std::array<char, AV_ERROR_MAX_STRING_SIZE> errstr;
    av_make_error_string(&errstr[0], errstr.size(), errnum);
    return &errstr[0];
}

static void grayscaleToYUV420_y(const uint8_t* grayscale, int width, int height, uint8_t* yChannel)
{
    /*
     * RGB -> YUV:
     *   Y  =      (0.257 * R) + (0.504 * G) + (0.098 * B) + 16
     *   Cr = V =  (0.439 * R) - (0.368 * G) - (0.071 * B) + 128
     *   Cb = U = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128
     *
     * Grayscale -> YUV:
     *   W = (R = G = B)
     *
     *   Y[i] = 0.895 * W[i] + 16.
     *   U[i] = 128 (fixed value)
     *   V[i] = 128 (fxied value)
     */

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            yChannel[y * width + x] = (uint8_t)(0.895 * (*grayscale++) + 16);
        }
    }
}

static void grayscaleToYUV420_uv(int width, int height, uint8_t* uChannel, uint8_t* vChannel)
{
    /*
     * RGB -> YUV:
     *   Y  =      (0.257 * R) + (0.504 * G) + (0.098 * B) + 16
     *   Cr = V =  (0.439 * R) - (0.368 * G) - (0.071 * B) + 128
     *   Cb = U = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128
     *
     * Grayscale -> YUV:
     *   W = (R = G = B)
     *
     *   Y[i] = 0.895 * W[i] + 16.
     *   U[i] = 128 (fixed value)
     *   V[i] = 128 (fxied value)
     */

    std::memset(uChannel, 128, width * height / 4);
    std::memset(vChannel, 128, width * height / 4);
}

static void encodeFrame(AVCodecContext*  ctx,
                        AVFrame*         frame,
                        AVPacket*        pkt,
                        AVStream*        stream,
                        AVFormatContext* out)
{
    if (auto r = avcodec_send_frame(ctx, frame); r < 0)
    {
        std::ostringstream msg;
        msg << "Could not send frame to encoder: " << av_strerror(r);
        throw std::runtime_error(msg.str());
    }

    while (true)
    {
        auto r = avcodec_receive_packet(ctx, pkt);
        if (r == AVERROR(EAGAIN) || r == AVERROR_EOF)
        {
            return;
        }
        else if (r < 0)
        {
            std::ostringstream msg;
            msg << "Could not encode frame: " << av_strerror(r);
            throw std::runtime_error(msg.str());
        }

        av_packet_rescale_ts(pkt, ctx->time_base, stream->time_base);

        av_write_frame(out, pkt);
        av_packet_unref(pkt);
    }
}

VideoWriter::VideoWriter(Config config)
: _cfg(config)
, _formatContext(nullptr)
, _videoStream(nullptr)
, _codecContext(nullptr)
, _videoFrame(nullptr)
, _videoPacket(nullptr)
, _videoFrameIndex(0)
{
}

VideoWriter::~VideoWriter()
{
    if (_videoPacket)
    {
        av_packet_free(&_videoPacket);
    }

    if (_videoFrame)
    {
        av_frame_free(&_videoFrame);
    }

    if (_codecContext)
    {
        avcodec_free_context(&_codecContext);
    }

    if (_formatContext)
    {
        avformat_free_context(_formatContext);
    }
}

static constexpr auto g_preferred_software_encoder = "libx265";

VideoWriter::VideoWriter(const std::string& filename, Config config)
: VideoWriter(config)
{
    // Set up for encoding a single video stream and storing it in filename with
    // the container format automatically detected based on the filename suffix.

    if (auto r = avformat_alloc_output_context2(&_formatContext, NULL, NULL, filename.c_str());
        r < 0)
    {
        std::ostringstream msg;
        msg << "Could not allocate video container state: " << av_strerror(r);
        throw std::runtime_error(msg.str());
    }

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

    _codecContext->width  = _cfg.width;
    _codecContext->height = _cfg.height;

    _codecContext->framerate = {_cfg.framerate.num, _cfg.framerate.den};
    _codecContext->time_base = {_cfg.framerate.den, _cfg.framerate.num};

    //
    // std::min(1, av_q2d(_codecContext->framerate) / 2);
    // AVRational{1, 2}

    _codecContext->gop_size     = 10;
    _codecContext->max_b_frames = 1;
    _codecContext->pix_fmt      = AV_PIX_FMT_YUV420P;

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

    if (!(_videoFrame = av_frame_alloc()))
    {
        throw std::runtime_error("Could not allocate video frame");
    }

    _videoFrame->format = _codecContext->pix_fmt;
    _videoFrame->width  = _codecContext->width;
    _videoFrame->height = _codecContext->height;

    if (auto r = av_frame_get_buffer(_videoFrame, 0); r < 0)
    {
        std::ostringstream msg;
        msg << "Could not allocate video frame data: " << av_strerror(r);
        throw std::runtime_error(msg.str());
    }

    // As we reuse the same frame, we only need to set the constant u and v channel once
    grayscaleToYUV420_uv(_videoFrame->width,
                         _videoFrame->height,
                         _videoFrame->data[1],
                         _videoFrame->data[2]);

    if (!(_videoPacket = av_packet_alloc()))
    {
        throw std::runtime_error("Could not allocate video packet");
    }

    if (auto r = avformat_write_header(_formatContext, NULL); r < 0)
    {
        std::ostringstream msg;
        msg << "Could not write video container header: " << av_strerror(r);
        throw std::runtime_error(msg.str());
    }
}

void VideoWriter::write(const ImageBuffer& image)
{
    if (auto r = av_frame_make_writable(_videoFrame); r < 0)
    {
        std::ostringstream msg;
        msg << "Could not make video frame writable: " << av_strerror(r);
        throw std::runtime_error(msg.str());
    }

    if ((image.width != _videoFrame->width) || (image.height != _videoFrame->height))
    {
        std::ostringstream msg;
        msg << "Could not write image: Got resolution " << image.width << "x" << image.height
            << " instead of " << _videoFrame->width << "x" << _videoFrame->height;
        throw std::runtime_error(msg.str());
    }

    grayscaleToYUV420_y(&image.data[0], image.width, image.height, _videoFrame->data[0]);
    _videoFrame->pts = _videoFrameIndex++;

    encodeFrame(_codecContext, _videoFrame, _videoPacket, _videoStream, _formatContext);
}

void VideoWriter::close()
{
    encodeFrame(_codecContext, nullptr, _videoPacket, _videoStream, _formatContext);

    if (auto r = av_write_trailer(_formatContext); r < 0)
    {
        std::ostringstream msg;
        msg << "Could not write video container trailer: " << av_strerror(r);
        throw std::runtime_error(msg.str());
    }
}
