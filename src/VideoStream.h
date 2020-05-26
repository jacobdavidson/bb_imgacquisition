// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <tuple>
#include <unordered_map>

#include <QString>

#include "ConcurrentQueue.h"
#include "GrayscaleImage.h"

class VideoStream final
{
public:
    using Image = GrayscaleImage;

    VideoStream(QString                                      id,
                std::tuple<size_t, size_t>                   resolution,
                float                                        _framesPerSecond,
                size_t                                       _framesPerFile,
                std::unordered_map<std::string, std::string> encoderOptions);

    void push(const Image& value);

    void push(Image&& value);

    void pop(Image& value);

    size_t size();

    const QString                                      id;
    const std::tuple<size_t, size_t>                   resolution;
    const float                                        framesPerSecond;
    const size_t                                       framesPerFile;
    const std::unordered_map<std::string, std::string> encoderOptions;

private:
    std::shared_ptr<ConcurrentQueue<Image>> _queue;
};
