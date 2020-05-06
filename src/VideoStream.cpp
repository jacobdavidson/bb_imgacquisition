// SPDX-License-Identifier: GPL-3.0-or-later

#include "VideoStream.h"

VideoStream::VideoStream(std::string                                  _id,
                         std::tuple<size_t, size_t>                   _resolution,
                         size_t                                       _framesPerSecond,
                         size_t                                       _framesPerFile,
                         std::unordered_map<std::string, std::string> _encoderOptions)
: id{_id}
, resolution{_resolution}
, framesPerSecond{_framesPerSecond}
, framesPerFile{_framesPerFile}
, encoderOptions{_encoderOptions}
, _queue{new ConcurrentQueue<std::shared_ptr<GrayscaleImage>>()}
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

size_t VideoStream::size()
{
    return _queue->size();
}
