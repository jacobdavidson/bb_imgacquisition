// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <iostream>
#include <fstream>

#include "settings/utility.h"
#include "settings/Settings.h"
// The order is important!
#include "VideoWriteThread.h"

#include "VideoFileWriter.h"

#include "format.h"

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
        if (maxSize == sizes.end() || *maxSize == 0)
        {
            usleep(500);
            continue;
        }

        auto videoStream           = _videoStreams[maxSizeIndex];
        const auto [width, height] = videoStream.resolution;

        const auto startTime = std::chrono::system_clock::now();

        namespace fs = boost::filesystem;

        const auto tmpDir = fs::path{set->tmpPath()} / videoStream.id;
        if (!fs::exists(tmpDir))
        {
            fs::create_directories(tmpDir);
        }

        const auto tmpVideoFilename = tmpDir / fmt::format("{}.mp4", startTime);

        VideoFileWriter f(tmpVideoFilename.string(),
                          {static_cast<int>(width),
                           static_cast<int>(height),
                           {static_cast<int>(videoStream.framesPerSecond), 1},
                           {_encoderName, videoStream.encoderOptions}});

        const auto tmpFrameTimestampsFilename = tmpDir / fmt::format("{}.txt", startTime);

        std::fstream frameTimestamps(tmpFrameTimestampsFilename, std::ios::trunc | std::ios::out);

        bool   videoStreamClosedEarly = false;
        size_t frameIndex             = 0;
        for (; frameIndex < videoStream.framesPerFile; frameIndex++)
        {
            std::shared_ptr<GrayscaleImage> img;
            videoStream.pop(img);
            if (!img)
            {
                videoStreamClosedEarly = true;
                break;
            }

            // Debug output TODO: remove?
            if (frameIndex % 100 == 0)
            {
                std::cout << "Loaded frame " << frameIndex << std::endl;
            }

            f.write(*img);

            if (frameTimestamps.is_open())
            {
                frameTimestamps << fmt::format("{}\n", img->timestamp);
                frameTimestamps.flush();
            }
        }

        f.close();
        frameTimestamps.close();

        const auto endTime = std::chrono::system_clock::now();

        if (!videoStreamClosedEarly)
        {
            try
            {
                const auto outDir = fs::path{set->outDirectory()} / videoStream.id;
                if (!fs::exists(outDir))
                {
                    fs::create_directories(outDir);
                    fs::permissions(outDir,
                                    fs::owner_all | fs::group_all | fs::others_read |
                                        fs::others_exe);
                }

                const auto outVideoFilename = outDir /
                                              fmt::format("{}--{}.mp4", startTime, endTime);

                fs::rename(tmpVideoFilename, outVideoFilename);
                fs::permissions(outVideoFilename,
                                fs::owner_read | fs::owner_write | fs::group_read |
                                    fs::group_write | fs::others_read | fs::others_write);

                const auto outFrameTimestampsFilename = outDir / fmt::format("{}--{}.txt",
                                                                             startTime,
                                                                             endTime);

                fs::rename(tmpFrameTimestampsFilename, outFrameTimestampsFilename);
                fs::permissions(outFrameTimestampsFilename,
                                fs::owner_read | fs::owner_write | fs::group_read |
                                    fs::group_write | fs::others_read | fs::others_write);
            }
            catch (const fs::filesystem_error& e)
            {
                std::cerr << e.what() << std::endl;
            }
        }
    }
}
