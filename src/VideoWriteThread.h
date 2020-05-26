// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>
#include <tuple>

#include <QThread>

#include "ImageStream.h"

/**
 * @brief The VideoWriteThread class
 *
 * This class reads from two ringbuffers
 * and processes the least occupied one
 * using the selected video encoder.
 *
 * Warning:
 * Keep in mind when using Nvidia hardware encoders:
 * -    The number of instances you spawn:
 *      Most consumer Nvidia GPUs are only able to spawn a limited amount of concurrent encoders,
 *      e.g. 2 for HEVC. One thread will use one encoder and crash if no GPU encoder is available.
 * -    The resolution:
 *      Encoding may not work correctly if 4096x4096 is exceeded.
 */
class VideoWriteThread final : public QThread
{
    Q_OBJECT

public:
    VideoWriteThread(std::string encoderName);

    void add(ImageStream imageStream);

private:
    const std::string        _encoderName;
    std::vector<ImageStream> _imageStreams;

    void run();
};
