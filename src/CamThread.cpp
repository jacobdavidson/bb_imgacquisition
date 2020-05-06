// SPDX-License-Identifier: GPL-3.0-or-later

#include "CamThread.h"

CamThread::CamThread(Config config, VideoStream videoStream, Watchdog* watchdog)
: _config{config}
, _videoStream{videoStream}
, _watchdog{watchdog}
{
}

CamThread::~CamThread()
{
}
