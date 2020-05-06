// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <tuple>
#include <unordered_map>

#include "ConcurrentQueue.h"
#include "GrayscaleImage.h"

class VideoStream final
{
public:
    using Image = std::shared_ptr<GrayscaleImage>;

    VideoStream(std::string                                  id,
                std::tuple<size_t, size_t>                   resolution,
                size_t                                       _framesPerSecond,
                size_t                                       _framesPerFile,
                std::unordered_map<std::string, std::string> encoderOptions);

    void push(const Image& value);

    void push(Image&& value);

    void pop(Image& value);

    size_t size();

    const std::string                                  id;
    const std::tuple<size_t, size_t>                   resolution;
    const size_t                                       framesPerSecond;
    const size_t                                       framesPerFile;
    const std::unordered_map<std::string, std::string> encoderOptions;

private:
    std::shared_ptr<ConcurrentQueue<std::shared_ptr<GrayscaleImage>>> _queue;
};
