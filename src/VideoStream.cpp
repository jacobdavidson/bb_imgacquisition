// SPDX-License-Identifier: GPL-3.0-or-later

#include "VideoStream.h"

VideoStream::VideoStream(std::string                                  _id,
                         std::tuple<std::size_t, std::size_t>         _resolution,
                         float                                        _framesPerSecond,
                         std::size_t                                  _framesPerFile,
                         std::unordered_map<std::string, std::string> _encoderOptions)
: id{_id}
, resolution{_resolution}
, framesPerSecond{_framesPerSecond}
, framesPerFile{_framesPerFile}
, encoderOptions{_encoderOptions}
, _queue{new ConcurrentQueue<Image>()}
{
}

void VideoStream::push(const Image& value)
{
    _queue->push(value);
}

void VideoStream::push(Image&& value)
{
    _queue->push(std::move(value));
}

void VideoStream::pop(Image& value)
{
    _queue->pop(value);
}

std::size_t VideoStream::size()
{
    return _queue->size();
}
