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
#include "VideoFileWriter.h"

VideoWriteThread::VideoWriteThread(std::string encoderName)
: _encoderName{encoderName}
{
}

void VideoWriteThread::add(VideoStream videoStream)
{
    _videoStreams.push_back(std::move(videoStream));
}

void VideoWriteThread::run()
{

    SettingsIAC* set = SettingsIAC::getInstance();

    auto imdir       = set->tmpPath();
    auto exchangedir = set->outDirectory();

    // For logging encoding times
    double elapsedTimeP, avgtimeP;

    while (true)
    {
        std::vector<size_t> sizes;
        for (auto& s : _videoStreams)
        {
            const auto [width, height] = s.resolution;
            sizes.emplace_back(s.size() * width * height);
        }

        const auto maxSize      = std::max_element(sizes.begin(), sizes.end());
        const auto maxSizeIndex = std::distance(sizes.begin(), maxSize);

        // If all are empty, check again later.
        if (*maxSize == 0)
        {
            usleep(500);
            continue;
        }

        // Configure output directories
        std::string dir   = imdir;
        std::string exdir = exchangedir;

        auto videoStream           = _videoStreams[maxSizeIndex];
        const auto [width, height] = videoStream.resolution;

        writeHandler wh(dir, videoStream.id, exdir);

        VideoFileWriter f(std::string(wh._videofile.c_str(), wh._videofile.size() - 4) + ".mp4",
                          {static_cast<int>(width),
                           static_cast<int>(height),
                           {static_cast<int>(videoStream.framesPerSecond), 1},
                           {_encoderName, videoStream.encoderOptions}});

        for (size_t frameIndex = 0; frameIndex < videoStream.framesPerFile; frameIndex++)
        {
            std::shared_ptr<GrayscaleImage> img;
            videoStream.pop(img);

            // Debug output TODO: remove?
            if (frameIndex % 100 == 0)
            {
                std::cout << "Loaded frame " << frameIndex << std::endl;
            }

            f.write(*img);

            // Log the progress to the writeHandler
            wh.log(img->timestamp);
        }
        f.close();
    }
}
