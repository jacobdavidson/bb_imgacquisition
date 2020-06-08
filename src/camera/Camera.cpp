// SPDX-License-Identifier: GPL-3.0-or-later

#include "Camera.hpp"

#include "util/format.hpp"
#include "util/log.hpp"

#include <opencv2/imgproc/imgproc.hpp>

#if defined(USE_FLEA3) && USE_FLEA3
    #include "Flea3Camera.hpp"
#endif

#if defined(USE_XIMEA) && USE_XIMEA
    #include "XimeaCamera.hpp"
#endif

#if defined(USE_BASLER) && USE_BASLER
    #include "BaslerCamera.hpp"
#endif

Camera::Camera(Config config, ImageStream imageStream)
: _config{config}
, _imageStream{imageStream}
{
}

Camera::~Camera()
{
    // Signal stream end to blocking consumer thread
    _imageStream.push({});
}

Camera* Camera::make(Config config, ImageStream imageStream)
{
#if defined(USE_FLEA3) && USE_FLEA3
    if (config.backend == "flea3")
    {
        return new Flea3Camera(config, imageStream);
    }
#endif

#if defined(USE_XIMEA) && USE_XIMEA
    if (config.backend == "ximea")
    {
        return new XimeaCamera(config, imageStream);
    }
#endif

#if defined(USE_BASLER) && USE_BASLER
    if (config.backend == "basler")
    {
        return new BaslerCamera(config, imageStream);
    }
#endif

    throw std::runtime_error(fmt::format("No such camera backend: {}", config.backend));
}

cv::Mat Camera::transform(int width, int height, void* data)
{
    auto image = cv::Mat{cv::Size{width, height}, CV_8UC1, data};

    if (width == _config.params.width && height == _config.params.height)
    {
        if (!(width == _config.width && height == _config.height))
        {
            image = image(
                cv::Rect{_config.offset_x, _config.offset_y, _config.width, _config.height});
        }
    }
    else
    {
        logCritical("{}: Camera captured image of incorrect size: {}x{}",
                    _imageStream.id,
                    width,
                    height);

        cv::resize(image, image, {_config.width, _config.height}, 0, 0, cv::INTER_LANCZOS4);
    }

    return image;
}
