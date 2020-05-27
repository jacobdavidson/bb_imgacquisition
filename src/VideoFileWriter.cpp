// SPDX-License-Identifier: GPL-3.0-or-later

#include "VideoFileWriter.hpp"

#include <array>
#include <sstream>
#include <cstring>
#include <string_view>

#include "util/format.hpp"

#if __has_include(<ffnvcodec/nvEncodeAPI.h>)
extern "C"
{
    #include <ffnvcodec/nvEncodeAPI.h>
}
#else
    #define NVENC_INFINITE_GOPLENGTH 0xffffffff
#endif

struct FFmpegInitializer final
{
    FFmpegInitializer()
    {

#if LIBAVFORMAT_VERSION_MAJOR < 58
        av_register_all();
#endif
        av_log_set_level(AV_LOG_WARNING);
    }
};
static FFmpegInitializer initFFmpeg;

static const char* av_strerror(int errnum)
{
    static thread_local std::array<char, AV_ERROR_MAX_STRING_SIZE> errstr;
    av_make_error_string(&errstr[0], errstr.size(), errnum);
    return &errstr[0];
}

static void grayscaleToYUV420_y(const std::uint8_t* grayscale,
                                int                 width,
                                int                 height,
                                std::uint8_t*       yChannel)
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
            yChannel[y * width + x] = (std::uint8_t)(0.895 * (*grayscale++) + 16);
        }
    }
}

static void grayscaleToYUV420_uv(int           width,
                                 int           height,
                                 std::uint8_t* uChannel,
                                 std::uint8_t* vChannel)
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

VideoFileWriter::VideoFileWriter(Config config)
: _cfg(config)
, _formatContext(nullptr)
, _imageStream(nullptr)
, _codecContext(nullptr)
, _videoFrame(nullptr)
, _videoPacket(nullptr)
, _videoFrameIndex(0)
{
}

VideoFileWriter::~VideoFileWriter()
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

VideoFileWriter::VideoFileWriter(const std::string& filename, Config config)
: VideoFileWriter(config)
{
    _filename = filename;

    // Set up for encoding a single image stream and storing it in filename with
    // the container format automatically detected based on the filename suffix.

    if (auto r = avformat_alloc_output_context2(&_formatContext, NULL, NULL, _filename.c_str());
        r < 0)
    {
        throw std::runtime_error(fmt::format("{}: Could not allocate video container state: {}",
                                             _filename,
                                             av_strerror(r)));
    }

    const auto* codec = avcodec_find_encoder_by_name(_cfg.codec.name.c_str());
    if (!codec)
    {
        throw std::runtime_error(fmt::format("Could not find video encoder: {}", _cfg.codec.name));
    }

    if (!avformat_query_codec(_formatContext->oformat, codec->id, FF_COMPLIANCE_VERY_STRICT))
    {
        throw std::runtime_error(
            fmt::format("{}: Cannot store {} encoded video in an {} container",
                        _filename,
                        codec->name,
                        _formatContext->oformat->name));
    }

    if (auto r = avio_open2(&_formatContext->pb, _filename.c_str(), AVIO_FLAG_WRITE, NULL, NULL);
        r < 0)
    {
        throw std::runtime_error(
            fmt::format("{}: Could not open for writing: {}", _filename, av_strerror(r)));
    }

    if (!(_imageStream = avformat_new_stream(_formatContext, codec)))
    {
        throw std::runtime_error(fmt::format("{}: Could not create new video stream", _filename));
    }

    if (!(_codecContext = avcodec_alloc_context3(codec)))
    {
        throw std::runtime_error(
            fmt::format("{}: Could not allocate video encoder state", _filename));
    }

    // Configure encoding

    _codecContext->width  = _cfg.width;
    _codecContext->height = _cfg.height;

    _codecContext->framerate = {_cfg.framerate.num, _cfg.framerate.den};
    _codecContext->time_base = {_cfg.framerate.den, _cfg.framerate.num};

    _codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    const auto codecName = std::string_view(codec->name);
    if (codecName == "nvenc_hevc")
    {
        throw std::runtime_error(fmt::format(
            "{}: Legacy nvenc_hevc encoder is not supported, use newer hevc_nvenc instead",
            _filename));
    }

    if (codecName == "hevc_nvenc" || codecName == "h264_nvenc")
    {
        _codecContext->bit_rate = 0;
    }

    for (const auto& [name, value] : _cfg.codec.options)
    {
        if (name == "goplength" && (codecName == "hevc_nvenc" || codecName == "h264_nvenc"))
        {
            if (value == "infinite")
            {
                _codecContext->gop_size = NVENC_INFINITE_GOPLENGTH;
            }
        }
        else
        {
            if (auto r = av_opt_set(_codecContext->priv_data, name.c_str(), value.c_str(), 0);
                r < 0)
            {
                throw std::runtime_error(
                    fmt::format("{}: Failed to set encoder option {} to {}: {}",
                                _filename,
                                name,
                                value,
                                av_strerror(r)));
            }
        }
    }

    if (_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
    {
        _codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    _imageStream->time_base      = _codecContext->time_base;
    _imageStream->avg_frame_rate = _codecContext->framerate;
    _imageStream->r_frame_rate   = _codecContext->framerate;

    if (auto r = avcodec_open2(_codecContext, codec, NULL); r < 0)
    {
        throw std::runtime_error(
            fmt::format("{}: Could not initialize video encoder: {}", _filename, av_strerror(r)));
    }

    if (auto r = avcodec_parameters_from_context(_imageStream->codecpar, _codecContext); r < 0)
    {
        throw std::runtime_error(
            fmt::format("{}: Could not copy video encoder settings to video stream: {}",
                        _filename,
                        av_strerror(r)));
    }

    if (!(_videoFrame = av_frame_alloc()))
    {
        throw std::runtime_error(fmt::format("{}: Could not allocate video frame", _filename));
    }

    _videoFrame->format = _codecContext->pix_fmt;
    _videoFrame->width  = _codecContext->width;
    _videoFrame->height = _codecContext->height;

    if (auto r = av_frame_get_buffer(_videoFrame, 0); r < 0)
    {
        throw std::runtime_error(
            fmt::format("{}: Could not allocate video frame data: {}", _filename, av_strerror(r)));
    }

    // As we reuse the same frame, we only need to set the constant u and v channel once
    grayscaleToYUV420_uv(_videoFrame->width,
                         _videoFrame->height,
                         _videoFrame->data[1],
                         _videoFrame->data[2]);

    if (!(_videoPacket = av_packet_alloc()))
    {
        throw std::runtime_error(fmt::format("{}: Could not allocate video packet", _filename));
    }

    if (auto r = avformat_write_header(_formatContext, NULL); r < 0)
    {
        throw std::runtime_error(fmt::format("{}: Could not write video container header: {}",
                                             _filename,
                                             av_strerror(r)));
    }
}

void VideoFileWriter::write(const GrayscaleImage& image)
{
    if (auto r = av_frame_make_writable(_videoFrame); r < 0)
    {
        throw std::runtime_error(
            fmt::format("{}: Could not make video frame writable: {}", _filename, av_strerror(r)));
    }

    if ((image.width != _videoFrame->width) || (image.height != _videoFrame->height))
    {
        throw std::runtime_error(fmt::format(
            "{}: Could not write image to video: Got resolution {}x{} instead of {}x{}",
            _filename,
            image.width,
            image.height,
            _videoFrame->width,
            _videoFrame->height));
    }

    grayscaleToYUV420_y(&image.data[0], image.width, image.height, _videoFrame->data[0]);
    _videoFrame->pts = _videoFrameIndex++;

    encodeFrame(_videoFrame);
}

void VideoFileWriter::close()
{
    encodeFrame(nullptr);

    if (auto r = av_write_trailer(_formatContext); r < 0)
    {
        throw std::runtime_error(fmt::format("{}: Could not write video container trailer: {}",
                                             _filename,
                                             av_strerror(r)));
    }
}

void VideoFileWriter::encodeFrame(AVFrame* frame)
{
    if (auto r = avcodec_send_frame(_codecContext, frame); r < 0)
    {
        throw std::runtime_error(
            fmt::format("Could not send frame to encoder: {}", av_strerror(r)));
    }

    while (true)
    {
        auto r = avcodec_receive_packet(_codecContext, _videoPacket);
        if (r == AVERROR(EAGAIN) || r == AVERROR_EOF)
        {
            return;
        }
        else if (r < 0)
        {
            throw std::runtime_error(fmt::format("Could not encode frame: {}", av_strerror(r)));
        }

        av_packet_rescale_ts(_videoPacket, _codecContext->time_base, _imageStream->time_base);

        av_write_frame(_formatContext, _videoPacket);
        av_packet_unref(_videoPacket);
    }
}
