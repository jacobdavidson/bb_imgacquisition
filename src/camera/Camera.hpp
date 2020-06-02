// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>
#include <string>
#include <optional>
#include <variant>

#include <boost/optional.hpp>

#include <QThread>

#include "ImageStream.hpp"

class Camera : public QThread
{
    Q_OBJECT

public:
    struct HardwareTrigger final
    {
        int source;
    };

    struct SoftwareTrigger final
    {
        float framesPerSecond;
    };

    struct Parameter_Auto final
    {
    };

    template<typename T>
    using Parameter_Manual = T;

    template<typename T>
    using Parameter = std::variant<Parameter_Auto, Parameter_Manual<T>>;

    struct Parameters final
    {
        int offset_x;
        int offset_y;
        int width;
        int height;

        std::variant<HardwareTrigger, SoftwareTrigger> trigger;

        std::optional<Parameter<float>> blacklevel;
        std::optional<Parameter<float>> exposure;
        std::optional<Parameter<float>> gain;

        boost::optional<int> buffer_size;
        boost::optional<int> throughput_limit;
    };

    struct Config final
    {
        std::string backend;
        std::string serial;

        Parameters params;

        int offset_x;
        int offset_y;
        int width;
        int height;
    };

protected:
    Camera(Config config, ImageStream imageStream);

    Config      _config;
    ImageStream _imageStream;

public:
    virtual ~Camera();

    static Camera* make(Config config, ImageStream imageStream);

signals:
    void imageCaptured(GrayscaleImage image);
};
