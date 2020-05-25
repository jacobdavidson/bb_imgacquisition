// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <type_traits>
#include <unordered_map>
#include <variant>

#include <string>
#include <iostream>

#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

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
        std::string id;

        struct Camera final
        {
            std::string backend;
            std::string serial;
            int         width;
            int         height;

            struct HardwareTrigger final
            {
                int source;
            };

            struct SoftwareTrigger final
            {
                size_t framesPerSecond;
            };

            std::variant<HardwareTrigger, SoftwareTrigger> trigger;

            boost::optional<int> buffer_size;
            boost::optional<int> throughput_limit;

            boost::optional<boost::optional<int>> blacklevel;
            boost::optional<boost::optional<int>> exposure;
            boost::optional<boost::optional<int>> shutter;
            boost::optional<boost::optional<int>> gain;
            boost::optional<bool>                 whitebalance;
        };

        Camera camera;

        size_t framesPerSecond;
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
