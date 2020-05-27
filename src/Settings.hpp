// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <type_traits>
#include <map>
#include <string>
#include <vector>

#include "camera/Camera.hpp"

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

    struct ImageStream final
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

    const std::vector<ImageStream>&           imageStreams() const;
    const std::map<std::string, std::string>& videoEncoders() const;

    const std::string temporaryDirectory() const;
    const std::string outputDirectory() const;

private:
    std::vector<ImageStream> _imageStreams;

    std::map<std::string, std::string> _videoEncoders;

    std::string _temporaryDirectory;
    std::string _outputDirectory;
};
