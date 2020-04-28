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

#include "nvenc/NvEncoder.h"

#include "writeHandler.h"

void VideoWriteThread::run()
{

    SettingsIAC* set = SettingsIAC::getInstance();

    std::string imdir       = set->getValueOfParam<std::string>(IMACQUISITION::IMDIR);
    std::string exchangedir = set->getValueOfParam<std::string>(IMACQUISITION::EXCHANGEDIR);

    // For logging encoding times
    double elapsedTimeP, avgtimeP;

    // Encoder may be reused. Potentially saves time.
    CNvEncoder           enc;
    EncoderQualityConfig cfgC1 = set->getBufferConf(_CamBuffer1);
    EncoderQualityConfig cfgC2 = set->getBufferConf(_CamBuffer2);

    while (1)
    {
        // Select a buffer to work on. Largest first.
        uint64_t c1 = _Buffer1->size() * (uint64_t)(cfgC1.width * cfgC1.height);
        uint64_t c2 = _Buffer2->size() * (uint64_t)(cfgC2.width * cfgC2.height);

        int                  currentCam = 0;
        MutexBuffer*         currentCamBuffer;
        EncoderQualityConfig encCfg;

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
            currentCamBuffer = _Buffer1;
            currentCam       = _CamBuffer1;
            encCfg           = cfgC1;
            std::cout << "Chosen: 1" << std::endl;
        }
        else if (c2 >= maxSize)
        {
            currentCamBuffer = _Buffer2;
            currentCam       = _CamBuffer2;
            encCfg           = cfgC2;
            std::cout << "Chosen: 2" << std::endl;
        }

        // Configure output directories
        std::string dir   = imdir;
        std::string exdir = exchangedir;

        writeHandler wh(dir, currentCam, exdir);

        // encode the frames in the buffer using given configuration
        std::cout << "Write handler initialized!" << std::endl;
        int ret = enc.EncodeMain(&elapsedTimeP, &avgtimeP, currentCamBuffer, &wh, encCfg);
        if (ret <= 0)
        {
            std::cerr << "ENCODER ERROR! " << std::endl;
        }
        else
        {
            std::cout << "Encoded " << ret << " byte" << std::endl;
        }
    }
}

VideoWriteThread::VideoWriteThread()
{

    SettingsIAC* set = SettingsIAC::getInstance();

    // Grab buffer size from json config and initialize buffers.
    _Buffer1    = new MutexLinkedList();
    _Buffer2    = new MutexLinkedList();
    _CamBuffer1 = -1;
    _CamBuffer2 = -1;
}
