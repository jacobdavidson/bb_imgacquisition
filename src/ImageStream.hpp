// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <tuple>
#include <unordered_map>
#include <string>

#include "util/ConcurrentQueue.hpp"
#include "GrayscaleImage.hpp"

class ImageStream final
{
public:
    using Image = GrayscaleImage;

    ImageStream(std::string                                  id,
                std::tuple<std::size_t, std::size_t>         resolution,
                float                                        _framesPerSecond,
                std::size_t                                  _framesPerFile,
                std::unordered_map<std::string, std::string> encoderOptions);

    void push(const Image& value);

    void push(Image&& value);

    void pop(Image& value);

    std::size_t size();

    const std::string                                  id;
    const std::tuple<std::size_t, std::size_t>         resolution;
    const float                                        framesPerSecond;
    const std::size_t                                  framesPerFile;
    const std::unordered_map<std::string, std::string> encoderOptions;

private:
    std::shared_ptr<ConcurrentQueue<Image>> _queue;
};
