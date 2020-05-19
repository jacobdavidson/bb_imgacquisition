// SPDX-License-Identifier: GPL-3.0-or-later

#include "CamThread.h"

#include "format.h"

#if defined(USE_FLEA3) && USE_FLEA3
    #include "Flea3CamThread.h"
#endif

#if defined(USE_XIMEA) && USE_XIMEA
    #include "XimeaCamThread.h"
#endif

#if defined(USE_BASLER) && USE_BASLER
    #include "BaslerCamThread.h"
#endif

CamThread::CamThread(Config config, VideoStream videoStream, Watchdog* watchdog)
: _config{config}
, _videoStream{videoStream}
, _watchdog{watchdog}
{
}

CamThread::~CamThread()
{
    // Signal stream end to blocking consumer thread
    _videoStream.push(nullptr);
}

CamThread* CamThread::make(Config config, VideoStream videoStream, Watchdog* watchdog)
{
#if defined(USE_FLEA3) && USE_FLEA3
    if (config.backend == "flea3")
    {
        return new Flea3CamThread(config, videoStream, watchdog);
    }
#endif

#if defined(USE_XIMEA) && USE_XIMEA
    if (config.backend == "ximea")
    {
        return new XimeaCamThread(config, videoStream, watchdog);
    }
#endif

#if defined(USE_BASLER) && USE_BASLER
    if (config.backend == "basler")
    {
        return new BaslerCamThread(config, videoStream, watchdog);
    }
#endif

    throw std::runtime_error(fmt::format("No such camera backend: {}", config.backend));
}
