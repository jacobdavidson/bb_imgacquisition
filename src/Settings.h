// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <type_traits>
#include <unordered_map>
#include <string>
#include <vector>

#include "camera/Camera.h"

class Settings
{
private:
    Settings();

    Settings(const Settings&)     = delete;
    Settings(Settings&&) noexcept = delete;
    Settings& operator=(const Settings&) = delete;
    Settings& operator=(Settings&&) noexcept = delete;

public:
    static const Settings& instance();

    struct VideoStream final
    {
        std::string id;

        Camera::Config camera;

        float       framesPerSecond;
        std::size_t framesPerFile;

        struct Encoder final
        {
            std::string                                  id;
            std::unordered_map<std::string, std::string> options;
        };

        Encoder encoder;
    };

    const std::vector<VideoStream>&                     videoStreams() const;
    const std::unordered_map<std::string, std::string>& videoEncoders() const;

    const std::string tmpDirectory() const;
    const std::string outDirectory() const;

private:
    std::vector<VideoStream> _videoStreams;

    std::unordered_map<std::string, std::string> _videoEncoders;

    std::string _tmpDirectory;
    std::string _outDirectory;
};
