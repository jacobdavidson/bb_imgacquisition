// SPDX-License-Identifier: GPL-3.0-or-later

#include "Settings.h"

#include "boost/program_options.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <sstream>

const boost::property_tree::ptree SettingsIAC::getDefaultParams()
{

    boost::property_tree::ptree pt;
    // std::string                 app = SettingsIAC::setConf("");

    // for (int i = 0; i < 4; i++)
    // {

    //     boost::property_tree::ptree hd;
    //     hd.put(IMACQUISITION::BUFFERCONF::CAMID, i);
    //     hd.put(IMACQUISITION::BUFFERCONF::SERIAL, 0);
    //     hd.put(IMACQUISITION::BUFFERCONF::SERIAL_STRING, "");
    //     hd.put(IMACQUISITION::BUFFERCONF::ENABLED, 1);
    //     hd.put(IMACQUISITION::BUFFERCONF::VIDEO_WIDTH, 4000);
    //     hd.put(IMACQUISITION::BUFFERCONF::VIDEO_HEIGHT, 3000);
    //     hd.put(IMACQUISITION::BUFFERCONF::BITRATE, 1000000);
    //     hd.put(IMACQUISITION::BUFFERCONF::RCMODE, 0);
    //     hd.put(IMACQUISITION::BUFFERCONF::QP, 25);
    //     hd.put(IMACQUISITION::BUFFERCONF::FRAMESPERVIDEO, 500);
    //     hd.put(IMACQUISITION::BUFFERCONF::FPS, 3);
    //     hd.put(IMACQUISITION::BUFFERCONF::PRESET, 2);
    //     hd.put(IMACQUISITION::BUFFERCONF::HWBUFSIZE, 0);
    //     hd.put(IMACQUISITION::BUFFERCONF::BRIGHTNESSONOFF, 1);
    //     hd.put(IMACQUISITION::BUFFERCONF::BRIGHTNESSAUTO, 0);
    //     hd.put(IMACQUISITION::BUFFERCONF::BRIGHTNESS, 0);
    //     hd.put(IMACQUISITION::BUFFERCONF::EXPOSUREONOFF, 0);
    //     hd.put(IMACQUISITION::BUFFERCONF::EXPOSUREAUTO, 0);
    //     hd.put(IMACQUISITION::BUFFERCONF::SHUTTER, 40);
    //     hd.put(IMACQUISITION::BUFFERCONF::SHUTTERONOFF, 1);
    //     hd.put(IMACQUISITION::BUFFERCONF::SHUTTERAUTO, 0);
    //     hd.put(IMACQUISITION::BUFFERCONF::GAIN, 0);
    //     hd.put(IMACQUISITION::BUFFERCONF::GAINONOFF, 1);
    //     hd.put(IMACQUISITION::BUFFERCONF::GAINAUTO, 0);
    //     hd.put(IMACQUISITION::BUFFERCONF::WHITEBALANCE, 0);
    //     hd.put(IMACQUISITION::BUFFERCONF::HWTRIGGER, 0);
    //     hd.put(IMACQUISITION::BUFFERCONF::HWTRIGGERPARAM, 0);
    //     hd.put(IMACQUISITION::BUFFERCONF::HWTRIGGERSOURCE, 0);

    //     boost::property_tree::ptree ld;
    //     ld.put(IMACQUISITION::BUFFERCONF::CAMID, i);
    //     ld.put(IMACQUISITION::BUFFERCONF::SERIAL, 0);
    //     ld.put(IMACQUISITION::BUFFERCONF::SERIAL_STRING, "");
    //     ld.put(IMACQUISITION::BUFFERCONF::ENABLED, 1);
    //     ld.put(IMACQUISITION::BUFFERCONF::VIDEO_WIDTH, 2000);
    //     ld.put(IMACQUISITION::BUFFERCONF::VIDEO_HEIGHT, 1500);
    //     ld.put(IMACQUISITION::BUFFERCONF::BITRATE, 1000000);
    //     ld.put(IMACQUISITION::BUFFERCONF::RCMODE, 0);
    //     ld.put(IMACQUISITION::BUFFERCONF::QP, 35);
    //     ld.put(IMACQUISITION::BUFFERCONF::FRAMESPERVIDEO, 500);
    //     ld.put(IMACQUISITION::BUFFERCONF::FPS, 3);
    //     ld.put(IMACQUISITION::BUFFERCONF::PRESET, 2);
    //     pt.add_child(IMACQUISITION::BUFFER, hd);
    //     pt.add_child(IMACQUISITION::BUFFER, ld);
    // }

    // pt.put(IMACQUISITION::LOGDIR, "./data/log/Cam_%d/");
    // pt.put(IMACQUISITION::IMDIR, "./data/tmp/Cam_%u/Cam_%u_%s--%s");
    // pt.put(IMACQUISITION::EXCHANGEDIR, "./data/out/Cam_%u/");
    // pt.put(IMACQUISITION::CAMCOUNT, 2);

    return pt;
}

void SettingsIAC::loadNewSettings()
{
    for (const auto& [videoStreamId, videoStreamTree] : _ptree.get_child("video_streams"))
    {
        VideoStream stream;

        stream.id = videoStreamId;

        const auto& cameraTree = videoStreamTree.get_child("camera");

        stream.camera.backend = cameraTree.get<std::string>("backend");
        stream.camera.serial = cameraTree.get<std::string>("serial");
        stream.camera.width  = cameraTree.get<int>("width");
        stream.camera.height = cameraTree.get<int>("height");

        const auto& triggerTree = cameraTree.get_child("trigger");

        const auto triggerType = triggerTree.get<std::string>("type");
        if (triggerType == "hardware")
        {
            VideoStream::Camera::HardwareTrigger trigger;
            trigger.source          = triggerTree.get<int>("source");
            stream.camera.trigger   = trigger;
        }
        else if (triggerType == "software")
        {
            VideoStream::Camera::SoftwareTrigger trigger;
            trigger.framesPerSecond = triggerTree.get<size_t>("frames_per_second");
            stream.camera.trigger   = trigger;
        }
        else
        {
            std::ostringstream msg;
            msg << "Invalid camera trigger type: " << triggerType;
            throw std::runtime_error(msg.str());
        }

        stream.camera.buffer_size = cameraTree.get_optional<int>("buffer_size");
        stream.camera.throughput_limit = cameraTree.get_optional<int>("throughput_limit");

        const auto parseAutoIntParam =
            [](const boost::property_tree::ptree& tree,
               const std::string&                 name) -> boost::optional<boost::optional<int>> {
            if (auto strValue = tree.get_optional<std::string>(name); strValue)
            {
                if (*strValue == "auto")
                {
                    return {boost::none};
                }
                else if (auto intValue = tree.get_optional<int>(name); intValue)
                {
                    return {{intValue}};
                }

                std::ostringstream msg;
                msg << "Invalid value for camera parameter \'" << name
                    << "\': Expected \"auto\" or integer value";
                throw std::runtime_error(msg.str());
            }

            return boost::none;
        };

        stream.camera.blacklevel = parseAutoIntParam(cameraTree, "blacklevel");
        stream.camera.exposure   = parseAutoIntParam(cameraTree, "exposure");
        stream.camera.shutter    = parseAutoIntParam(cameraTree, "shutter");
        stream.camera.gain       = parseAutoIntParam(cameraTree, "gain");

        stream.camera.whitebalance = cameraTree.get_optional<bool>("whitebalance");

        stream.framesPerSecond = videoStreamTree.get<size_t>("frames_per_second");
        stream.framesPerFile = videoStreamTree.get<size_t>("frames_per_file");

        const auto& encoderTree = videoStreamTree.get_child("encoder");
        stream.encoder.id = encoderTree.get<std::string>("id");

        for (const auto& [key, value] : encoderTree.get_child("options"))
        {
            stream.encoder.options.emplace(key, value.get_value<std::string>());
        }

        _videoStreams.push_back(stream);
    }

    for (auto& [id, name] : _ptree.get_child("video_encoders"))
    {
        _videoEncoders.emplace(id, name.get_value<std::string>());
    }

    _logDirectory = _ptree.get<std::string>("log_directory");
    _tmpPath      = _ptree.get<std::string>("tmp_path");
    _outDirectory = _ptree.get<std::string>("out_directory");
}

const std::vector<SettingsIAC::VideoStream>& SettingsIAC::videoStreams() const
{
    return _videoStreams;
}

const std::unordered_map<std::string, std::string>& SettingsIAC::videoEncoders() const
{
    return _videoEncoders;
}

const std::string SettingsIAC::logDirectory() const
{
    return _logDirectory;
}

const std::string SettingsIAC::tmpPath() const
{
    return _tmpPath;
}

const std::string SettingsIAC::outDirectory() const
{
    return _outDirectory;
}
