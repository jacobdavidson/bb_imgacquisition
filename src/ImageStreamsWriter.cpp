// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <fstream>

#include <boost/filesystem.hpp>

#include "Settings.h"
// NOTE: The order of the includes below is important to avoid name clashes!
#include "ImageStreamsWriter.h"

#include "VideoFileWriter.h"

#include "util/format.h"
#include "util/log.h"

ImageStreamsWriter::ImageStreamsWriter(std::string encoderName)
: _encoderName{encoderName}
{
}

void ImageStreamsWriter::add(ImageStream imageStream)
{
    _imageStreams.push_back(std::move(imageStream));
}

void ImageStreamsWriter::run()
{
    const auto& settings = Settings::instance();

    while (!isInterruptionRequested())
    {
        std::vector<std::size_t> sizes;
        for (auto& s : _imageStreams)
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

        auto imageStream           = _imageStreams[maxSizeIndex];
        const auto [width, height] = imageStream.resolution;

        const auto startTime = std::chrono::system_clock::now();

        namespace fs = boost::filesystem;

        const auto tmpDir = fs::path{settings.tmpDirectory()} / imageStream.id;
        if (!fs::exists(tmpDir))
        {
            fs::create_directories(tmpDir);
        }

        const auto tmpVideoFilename = tmpDir / fmt::format("{}.mp4", startTime);

        // FIXME: framesPerSecond is a float, should be properly converted to rational
        VideoFileWriter f(tmpVideoFilename.string(),
                          {static_cast<int>(width),
                           static_cast<int>(height),
                           {static_cast<int>(imageStream.framesPerSecond), 1},
                           {_encoderName, imageStream.encoderOptions}});
        logDebug("{}: New video file", tmpVideoFilename);

        const auto tmpFrameTimestampsFilename = tmpDir / fmt::format("{}.txt", startTime);

        std::fstream frameTimestamps(tmpFrameTimestampsFilename.string(),
                                     std::ios::trunc | std::ios::out);

        const std::size_t debugInterval = 100;

        bool        imageStreamClosedEarly = false;
        std::size_t frameIndex             = 0;
        for (; frameIndex < imageStream.framesPerFile; frameIndex++)
        {
            ImageStream::Image img;
            imageStream.pop(img);
            if (img.data.empty())
            {
                imageStreamClosedEarly = true;
                break;
            }

            f.write(img);

            if (frameIndex % debugInterval == 0)
            {
                logDebug("{}: Wrote video frame {}", tmpVideoFilename, frameIndex);
            }

            if (frameTimestamps.is_open())
            {
                frameTimestamps << fmt::format("{:e.6}\n", img.timestamp);
                frameTimestamps.flush();
            }
        }

        f.close();
        frameTimestamps.close();

        const auto endTime = std::chrono::system_clock::now();

        if (!imageStreamClosedEarly)
        {
            try
            {
                const auto outDir = fs::path{settings.outDirectory()} / imageStream.id;
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

                logInfo("{}: Finished", outVideoFilename);
            }
            catch (const fs::filesystem_error& e)
            {
                logCritical("{}: Failed to finalize video file: {}", tmpVideoFilename, e.what());
            }
        }
        else
        {
            logInfo("{}: Interrupted", tmpVideoFilename);
        }
    }
}
