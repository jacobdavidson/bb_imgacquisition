// SPDX-License-Identifier: GPL-3.0-or-later

#include "Camera.h"

#include "util/format.h"

#if defined(USE_FLEA3) && USE_FLEA3
    #include "Flea3Camera.h"
#endif

#if defined(USE_XIMEA) && USE_XIMEA
    #include "XimeaCamera.h"
#endif

#if defined(USE_BASLER) && USE_BASLER
    #include "BaslerCamera.h"
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
