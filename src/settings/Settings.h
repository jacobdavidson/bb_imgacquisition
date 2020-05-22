// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <type_traits>
#include <unordered_map>
#include <variant>

#include <string>
#include <fstream>
#include <iostream>

#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

class SettingsIAC
{

private:
    void loadNewSettings();

    /* This is a singleton. Get it using something like:
     * SettingsIAC *myInstance = SettingsIAC::getInstance();
     */
    SettingsIAC()
    {
        // Setting default file, if unset
        std::string confFile = setConf("");
        if (confFile == "")
            confFile = "configImAcq.json";

        std::ifstream conf(confFile.c_str());
        if (conf.good())
        {
            boost::property_tree::read_json(confFile, _ptree);
            loadNewSettings();
        }
        else
        {
            _ptree = getDefaultParams();
            boost::property_tree::write_json(confFile, _ptree);
            std::cout << "**********************************" << std::endl;
            std::cout << "* Created default configuration. *" << std::endl;
            std::cout << "* Please adjust the config or    *" << std::endl;
            std::cout << "* and run again. You might also  *" << std::endl;
            std::cout << "* just stick with the defaults.  *" << std::endl;
            std::cout << "**********************************" << std::endl;
            exit(0);
        }
    }

    // C++ 11 style
    // =======
    SettingsIAC(SettingsIAC const&) = delete;
    void operator=(SettingsIAC const&) = delete;

public:
    // To set options from CLI
    boost::property_tree::ptree _ptree;

    // Make the config file adjustable. Retrieve via empty string.
    // The config file will determine which app is being run.
    // This majorly influences the default settings.
    // See Settings.cpp and ParamNames.h for details.
    static std::string setConf(std::string c)
    {
        static std::string confFile;
        if (c != "")
            confFile = c;
        return confFile;
    }

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

    const std::string tmpPath() const;
    const std::string outDirectory() const;

private:
    static const boost::property_tree::ptree getDefaultParams();

    std::vector<VideoStream> _videoStreams;

    std::unordered_map<std::string, std::string> _videoEncoders;

    std::string _tmpPath;
    std::string _outDirectory;
};
