// SPDX-License-Identifier: GPL-3.0-or-later

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

#include "WriteHandler.h"
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

    while (!isInterruptionRequested())
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

        WriteHandler wh(dir, videoStream.id, exdir);

        VideoFileWriter f(std::string(wh._videofile.c_str(), wh._videofile.size() - 4) + ".mp4",
                          {static_cast<int>(width),
                           static_cast<int>(height),
                           {static_cast<int>(videoStream.framesPerSecond), 1},
                           {_encoderName, videoStream.encoderOptions}});

        size_t frameIndex = 0;
        for (; frameIndex < videoStream.framesPerFile; frameIndex++)
        {
            std::shared_ptr<GrayscaleImage> img;
            videoStream.pop(img);
            if (!img)
            {
                wh._skipFinalization = true;
                break;
            }

            // Debug output TODO: remove?
            if (frameIndex % 100 == 0)
            {
                std::cout << "Loaded frame " << frameIndex << std::endl;
            }

            f.write(*img);

            // Log the progress to the WriteHandler
            wh.log(img->timestamp);
        }

        f.close();
    }
}
