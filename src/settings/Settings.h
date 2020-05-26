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

#include <QString>

class SettingsIAC
{

private:
    const boost::property_tree::ptree detectSettings() const;

    void loadNewSettings();

    SettingsIAC();

    // C++ 11 style
    // =======
    SettingsIAC(SettingsIAC const&) = delete;
    void operator=(SettingsIAC const&) = delete;

public:
    // To set options from CLI
    boost::property_tree::ptree _ptree;

    static SettingsIAC* getInstance()
    {
        static SettingsIAC instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return &instance;
    }

    struct VideoStream final
    {
        QString id;

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
    static const boost::property_tree::ptree getDefaultParams();

    std::vector<VideoStream> _videoStreams;

    std::unordered_map<std::string, std::string> _videoEncoders;

    std::string _tmpDirectory;
    std::string _outDirectory;
};
