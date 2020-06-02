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

    struct Encoder final
    {
        std::string                                  id;
        std::unordered_map<std::string, std::string> options;
    };

    struct Stream final
    {
        std::string id;

        Camera::Config camera;

        float       framesPerSecond;
        std::size_t framesPerFile;

        Encoder encoder;
    };

    const std::vector<Stream>&                streams() const;
    const std::map<std::string, std::string>& encoders() const;

    const std::string temporaryDirectory() const;
    const std::string outputDirectory() const;

private:
    std::string _temporaryDirectory;
    std::string _outputDirectory;

    std::map<std::string, std::string> _encoders;

    std::vector<Stream> _streams;
};
