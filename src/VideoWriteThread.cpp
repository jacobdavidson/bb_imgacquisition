// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * VideoWriteThread.cpp
 *
 *  Created on: Nov 6, 2015
 *      Author: hauke
 */

#include <time.h>

#if HAVE_UNISTD_H
    #include <unistd.h> //sleep
#else
    #include <stdint.h>
#endif
#include <iostream>
#include "settings/utility.h"
#include "settings/Settings.h"
// The order is important!
#include "VideoWriteThread.h"

#include "writeHandler.h"
#include "VideoWriter.h"

void VideoWriteThread::run()
{

    SettingsIAC* set = SettingsIAC::getInstance();

    std::string imdir       = set->getValueOfParam<std::string>(IMACQUISITION::IMDIR);
    std::string exchangedir = set->getValueOfParam<std::string>(IMACQUISITION::EXCHANGEDIR);

    // For logging encoding times
    double elapsedTimeP, avgtimeP;

    EncoderQualityConfig cfgC1 = set->getBufferConf(_CamBuffer1);
    EncoderQualityConfig cfgC2 = set->getBufferConf(_CamBuffer2);

    while (1)
    {
        // Select a buffer to work on. Largest first.
        uint64_t c1 = _Buffer1.size() * (uint64_t)(cfgC1.width * cfgC1.height);
        uint64_t c2 = _Buffer2.size() * (uint64_t)(cfgC2.width * cfgC2.height);

        int                                               currentCam = 0;
        ConcurrentQueue<std::shared_ptr<GrayscaleImage>>* currentCamBuffer;
        EncoderQualityConfig                              encCfg;

        auto maxSize = std::max({c1, c2});

        // If all are empty, check again later.
        if (maxSize == 0)
        {
            usleep(500);
            continue;
        }

        // Configure which buffer and configuration to use
        if (c1 >= maxSize)
        {
            currentCamBuffer = &_Buffer1;
            currentCam       = _CamBuffer1;
            encCfg           = cfgC1;
            std::cout << "Chosen: 1" << std::endl;
        }
        else if (c2 >= maxSize)
        {
            currentCamBuffer = &_Buffer2;
            currentCam       = _CamBuffer2;
            encCfg           = cfgC2;
            std::cout << "Chosen: 2" << std::endl;
        }

        // Configure output directories
        std::string dir   = imdir;
        std::string exdir = exchangedir;

        writeHandler wh(dir, currentCam, exdir);

        VideoWriter videoWriter(
            std::string(wh._videofile.c_str(), wh._videofile.size() - 4) + ".mp4",
            {encCfg.width,
             encCfg.height,
             {encCfg.fps, 1},
             {"hevc_nvenc", {{"preset", "default"}, {"rc", "vbr_hq"}, {"cq", "25"}}}});

        for (int frm = 0; frm < encCfg.totalFrames; frm++)
        {
            std::shared_ptr<GrayscaleImage> img;
            currentCamBuffer->pop(img);

            // Debug output TODO: remove?
            if (frm % 100 == 0)
            {
                std::cout << "Loaded frame " << frm << std::endl;
            }

            videoWriter.write(*img);

            // Log the progress to the writeHandler
            wh.log(img->timestamp);
        }
        videoWriter.close();
    }
}

VideoWriteThread::VideoWriteThread()
: _CamBuffer1(-1)
, _CamBuffer2(-1)
{
}
