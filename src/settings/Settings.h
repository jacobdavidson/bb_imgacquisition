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

#include "ParamNames.h"

/*rcmode		The rc (rate control) mode to use. <br>
 * 						0 = Constant quantizer parameter (default)<br>
 * 						1 = VBR mode<br>
 * 						2 = CBR mode<br>
 * 						3 = VBR mode using minimum quantizer parameter<br>
 * 						4 = 2-pass quality<br>
 * 						5 = 2-pass frame size cap<br>
 * 						6 = 2-pass VBR
 * preset		NvEncoder preset. <br>
 * 						0 = Default<br>
 * 						1 = High performance<br>
 * 						2 = High quality<br>
 * 						3 = BD<br>
 * 						4 = Low latency default<br>
 * 						5 = Low latency high quality<br>
 * 						6 = Low latency high performance<br>
 * 						7 = Lossless default<br>
 * 						8 = Lossless high performance.<br>
 * 						Please see NvEncoder documentation for what
 * 						hardware is supporting lossless.
 * qp			The quantizer parameter
 * bitrate		Desired bitrate. Irrelevant for constant qp.
 * totalFrames	Total number of frames to encode.
 * fps			Framerate of target video.
 * width			Width of the input and output. No scaling is done.<br>
 * 						Please note that NvEnc only supports up to 4096x4096.
 * height		Height of the input and output. No scaling is done.<br>
 * 						Please note that NvEnc only supports up to 4096x4096.
 * TODO: More parameters
 */
typedef struct _EncoderQualityConfig
{
    int         camid;
    int         serial;
    std::string serialString;
    int         enabled;
    int         rcmode;
    int         preset;
    int         qp;
    int         bitrate;
    int         totalFrames;
    int         width;
    int         height;
    int         fps;
    int         hwbuffersize;
    int         brightness;
    int         brightnessonoff;
    int         brightnessauto;
    int         exposureonoff;
    int         exposureauto;
    int         shutter;
    int         shutteronoff;
    int         shutterauto;
    int         gain;
    int         gainonoff;
    int         gainauto;
    int         whitebalance;
    int         hwtrigger;
    int         hwtriggerparam;
    int         hwtriggersrc;
} EncoderQualityConfig;

namespace
{
    template<typename Test, template<typename...> class Ref>
    struct is_specialization : std::false_type
    {
    };

    template<template<typename...> class Ref, typename... Args>
    struct is_specialization<Ref<Args...>, Ref> : std::true_type
    {
    };
}

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
            confFile = CONFIGPARAM::CONFIGURATION_FILE;

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

    /**
     * Sets the parameter.
     * @param paramName name of the parameter,
     * @param paramValue value of the parameter,
     */
    template<typename T>
    void setParam(std::string const& paramName, T&& paramValue)
    {
        _ptree.put(paramName, std::forward<T>(paramValue));
        boost::property_tree::write_json(CONFIGPARAM::CONFIGURATION_FILE, _ptree);
    }

    /**
     * Sets the vector of values of a parameter.
     * @param paramName name of the parameter,
     * @param paramVector vector of values of the parameter,
     */
    template<typename T>
    void setParam(std::string const& paramName, std::vector<T>&& paramVector)
    {
        boost::property_tree::ptree subtree;
        for (T& value : paramVector)
        {
            boost::property_tree::ptree valuetree;
            valuetree.put("", value);
            subtree.push_back(std::make_pair("", valuetree));
        }
        _ptree.put_child(paramName, subtree);
        boost::property_tree::write_json(CONFIGPARAM::CONFIGURATION_FILE, _ptree);
    }

    /**
     * Gets the parameter value provided by parameter name.
     * @param paramName the parameter name,
     * @return the value of the parameter as the specified type.
     */
    template<typename T>
    typename std::enable_if<!is_specialization<T, std::vector>::value, T>::type getValueOfParam(
        const std::string& paramName) const
    {
        return _ptree.get<T>(paramName);
    }

    /**
     * Gets the vector of values provided by parameter name.
     *
     * Throws an boost exception if parameter does not exist.
     *
     * @param paramName the parameter name,
     * @return the vector of values of the parameter with the specified type.
     */
    template<typename T>
    typename std::enable_if<is_specialization<T, std::vector>::value, T>::type getValueOfParam(
        const std::string& paramName) const
    {
        T result;
        for (auto& item : _ptree.get_child(paramName))
        {
            result.push_back(item.second.get_value<typename T::value_type>());
        }
        return result;
    }

    /**
     * Gets either the parameter value provided by parameter name, if it
     * exists, or a empty boost::optional<T> otherwise.
     * @param paramName the parameter name,
     * @return the value of the parameter wrapped in a boost::optional.
     */
    template<typename T>
    boost::optional<T> maybeGetValueOfParam(const std::string& paramName) const
    {
        return _ptree.get_optional<T>(paramName);
    }

    /**
     * Gets the parameter value provided by parameter name.
     * If the parameter is not set yet, set to default value and return it.
     * @param paramName the parameter name,
     * @param defaultValue the default parameter value,
     * @return the value of the parameter as the specified type.
     */
    template<typename T>
    T getValueOrDefault(const std::string& paramName, const T& defaultValue)
    {
        boost::optional<T> value = maybeGetValueOfParam<T>(paramName);
        if (value)
        {
            return value.get();
        }
        else
        {
            setParam(paramName, defaultValue);
            return defaultValue;
        }
    }

    EncoderQualityConfig getBufferConf(int camid);

    struct VideoStream final
    {
        struct Camera final
        {
            std::string serial;
            int         id;
            int         width;
            int         height;

            struct HardwareTrigger final
            {
                int source;
                int framesPerSecond;
            };

            struct SoftwareTrigger final
            {
                int framesPerSecond;
            };

            std::variant<HardwareTrigger, SoftwareTrigger> trigger;

            boost::optional<int> buffersize;

            boost::optional<boost::optional<int>>  brightness;
            boost::optional<boost::optional<int>>  exposure;
            boost::optional<boost::optional<int>>  shutter;
            boost::optional<boost::optional<int>>  gain;
            boost::optional<bool> whitebalance;
        };

        Camera camera;

        int framesPerVideoFile;

        struct
        {
            std::string                                  name;
            std::unordered_map<std::string, std::string> options;
        } encoder;
    };

    const std::vector<VideoStream>& videoStreams() const;

    const std::string logDirectory() const;
    const std::string tmpPath() const;
    const std::string outDirectory() const;

private:
    EncoderQualityConfig setFromNode(boost::property_tree::ptree node);

    static const boost::property_tree::ptree getDefaultParams();

    std::vector<VideoStream> _videoStreams;

    std::string _logDirectory;
    std::string _tmpPath;
    std::string _outDirectory;
};
