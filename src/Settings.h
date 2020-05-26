// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <type_traits>
#include <unordered_map>
#include <variant>
#include <optional>
#include <string>

#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

class Settings
{

private:
    const boost::property_tree::ptree detectSettings() const;

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

        struct Camera final
        {
            std::string backend;
            std::string serial;

            int offset_x;
            int offset_y;
            int width;
            int height;

            struct HardwareTrigger final
            {
                int source;
            };

            struct SoftwareTrigger final
            {
                float framesPerSecond;
            };

            struct Parameter_Auto final
            {
            };

            template<typename T>
            using Parameter_Manual = T;

            template<typename T>
            using Parameter = std::variant<Parameter_Auto, Parameter_Manual<T>>;

            std::variant<HardwareTrigger, SoftwareTrigger> trigger;

            boost::optional<int> buffer_size;
            boost::optional<int> throughput_limit;

            std::optional<Parameter<float>> blacklevel;
            std::optional<Parameter<float>> exposure;
            std::optional<Parameter<float>> gain;
        };

        Camera camera;

        float  framesPerSecond;
        size_t framesPerFile;

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
    boost::property_tree::ptree _tree;

    static const boost::property_tree::ptree getDefaultParams();

    std::vector<VideoStream> _videoStreams;

    std::unordered_map<std::string, std::string> _videoEncoders;

    std::string _tmpDirectory;
    std::string _outDirectory;
};
