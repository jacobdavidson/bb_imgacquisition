// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>
#include <tuple>

#include <QThread>

#include "ImageStream.hpp"

/**
 * @brief Writes image streams to respective video files with a single video encoder
 *
 * Keep in mind when using Nvidia hardware encoders:
 * -    Most consumer Nvidia GPUs are only able to spawn a limited amount of concurrent video
 * encoders.
 * -    Encoding may not work correctly if a resolution of 4096x4096 pixels is exceeded.
 */
class ImageStreamsWriter final : public QThread
{
    Q_OBJECT

public:
    ImageStreamsWriter(std::string encoderName);

    void add(ImageStream imageStream);

private:
    const std::string        _encoderName;
    std::vector<ImageStream> _imageStreams;

    void run();
};
