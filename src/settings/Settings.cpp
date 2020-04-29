// SPDX-License-Identifier: GPL-3.0-or-later

#include "Settings.h"

#include "boost/program_options.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <sstream>

const boost::property_tree::ptree SettingsIAC::getDefaultParams()
{

    boost::property_tree::ptree pt;
    std::string                 app = SettingsIAC::setConf("");

    for (int i = 0; i < 4; i++)
    {

        boost::property_tree::ptree hd;
        hd.put(IMACQUISITION::BUFFERCONF::CAMID, i);
        hd.put(IMACQUISITION::BUFFERCONF::SERIAL, 0);
        hd.put(IMACQUISITION::BUFFERCONF::SERIAL_STRING, "");
        hd.put(IMACQUISITION::BUFFERCONF::ENABLED, 1);
        hd.put(IMACQUISITION::BUFFERCONF::VIDEO_WIDTH, 4000);
        hd.put(IMACQUISITION::BUFFERCONF::VIDEO_HEIGHT, 3000);
        hd.put(IMACQUISITION::BUFFERCONF::BITRATE, 1000000);
        hd.put(IMACQUISITION::BUFFERCONF::RCMODE, 0);
        hd.put(IMACQUISITION::BUFFERCONF::QP, 25);
        hd.put(IMACQUISITION::BUFFERCONF::FRAMESPERVIDEO, 500);
        hd.put(IMACQUISITION::BUFFERCONF::FPS, 3);
        hd.put(IMACQUISITION::BUFFERCONF::PRESET, 2);
        hd.put(IMACQUISITION::BUFFERCONF::HWBUFSIZE, 0);
        hd.put(IMACQUISITION::BUFFERCONF::BRIGHTNESSONOFF, 1);
        hd.put(IMACQUISITION::BUFFERCONF::BRIGHTNESSAUTO, 0);
        hd.put(IMACQUISITION::BUFFERCONF::BRIGHTNESS, 0);
        hd.put(IMACQUISITION::BUFFERCONF::EXPOSUREONOFF, 0);
        hd.put(IMACQUISITION::BUFFERCONF::EXPOSUREAUTO, 0);
        hd.put(IMACQUISITION::BUFFERCONF::SHUTTER, 40);
        hd.put(IMACQUISITION::BUFFERCONF::SHUTTERONOFF, 1);
        hd.put(IMACQUISITION::BUFFERCONF::SHUTTERAUTO, 0);
        hd.put(IMACQUISITION::BUFFERCONF::GAIN, 0);
        hd.put(IMACQUISITION::BUFFERCONF::GAINONOFF, 1);
        hd.put(IMACQUISITION::BUFFERCONF::GAINAUTO, 0);
        hd.put(IMACQUISITION::BUFFERCONF::WHITEBALANCE, 0);
        hd.put(IMACQUISITION::BUFFERCONF::HWTRIGGER, 0);
        hd.put(IMACQUISITION::BUFFERCONF::HWTRIGGERPARAM, 0);
        hd.put(IMACQUISITION::BUFFERCONF::HWTRIGGERSOURCE, 0);

        boost::property_tree::ptree ld;
        ld.put(IMACQUISITION::BUFFERCONF::CAMID, i);
        ld.put(IMACQUISITION::BUFFERCONF::SERIAL, 0);
        ld.put(IMACQUISITION::BUFFERCONF::SERIAL_STRING, "");
        ld.put(IMACQUISITION::BUFFERCONF::ENABLED, 1);
        ld.put(IMACQUISITION::BUFFERCONF::VIDEO_WIDTH, 2000);
        ld.put(IMACQUISITION::BUFFERCONF::VIDEO_HEIGHT, 1500);
        ld.put(IMACQUISITION::BUFFERCONF::BITRATE, 1000000);
        ld.put(IMACQUISITION::BUFFERCONF::RCMODE, 0);
        ld.put(IMACQUISITION::BUFFERCONF::QP, 35);
        ld.put(IMACQUISITION::BUFFERCONF::FRAMESPERVIDEO, 500);
        ld.put(IMACQUISITION::BUFFERCONF::FPS, 3);
        ld.put(IMACQUISITION::BUFFERCONF::PRESET, 2);
        pt.add_child(IMACQUISITION::BUFFER, hd);
        pt.add_child(IMACQUISITION::BUFFER, ld);
    }

    pt.put(IMACQUISITION::LOGDIR, "./data/log/Cam_%d/");
    pt.put(IMACQUISITION::IMDIR, "./data/tmp/Cam_%u/Cam_%u_%s--%s");
    pt.put(IMACQUISITION::EXCHANGEDIR, "./data/out/Cam_%u/");
    pt.put(IMACQUISITION::CAMCOUNT, 2);

    return pt;
}

EncoderQualityConfig SettingsIAC::setFromNode(boost::property_tree::ptree node)
{
    EncoderQualityConfig cfg;

    cfg.camid        = node.get<int>(IMACQUISITION::BUFFERCONF::CAMID);
    cfg.serial       = node.get<int>(IMACQUISITION::BUFFERCONF::SERIAL);
    cfg.serialString = node.get<std::string>(IMACQUISITION::BUFFERCONF::SERIAL_STRING);
    cfg.enabled      = node.get<int>(IMACQUISITION::BUFFERCONF::ENABLED);
    cfg.rcmode       = node.get<int>(IMACQUISITION::BUFFERCONF::RCMODE);
    cfg.preset       = node.get<int>(IMACQUISITION::BUFFERCONF::PRESET);
    cfg.qp           = node.get<int>(IMACQUISITION::BUFFERCONF::QP);
    cfg.bitrate      = node.get<int>(IMACQUISITION::BUFFERCONF::BITRATE);
    cfg.totalFrames  = node.get<int>(IMACQUISITION::BUFFERCONF::FRAMESPERVIDEO);
    cfg.width        = node.get<int>(IMACQUISITION::BUFFERCONF::VIDEO_WIDTH);
    cfg.height       = node.get<int>(IMACQUISITION::BUFFERCONF::VIDEO_HEIGHT);
    cfg.fps          = node.get<int>(IMACQUISITION::BUFFERCONF::FPS);

    cfg.hwbuffersize    = node.get<int>(IMACQUISITION::BUFFERCONF::HWBUFSIZE);
    cfg.brightness      = node.get<int>(IMACQUISITION::BUFFERCONF::BRIGHTNESS);
    cfg.brightnessonoff = node.get<int>(IMACQUISITION::BUFFERCONF::BRIGHTNESSONOFF);
    cfg.brightnessauto  = node.get<int>(IMACQUISITION::BUFFERCONF::BRIGHTNESSAUTO);
    cfg.exposureonoff   = node.get<int>(IMACQUISITION::BUFFERCONF::EXPOSUREONOFF);
    cfg.exposureauto    = node.get<int>(IMACQUISITION::BUFFERCONF::EXPOSUREAUTO);
    cfg.shutter         = node.get<int>(IMACQUISITION::BUFFERCONF::SHUTTER);
    cfg.shutteronoff    = node.get<int>(IMACQUISITION::BUFFERCONF::SHUTTERONOFF);
    cfg.shutterauto     = node.get<int>(IMACQUISITION::BUFFERCONF::SHUTTERAUTO);
    cfg.gain            = node.get<int>(IMACQUISITION::BUFFERCONF::GAIN);
    cfg.gainonoff       = node.get<int>(IMACQUISITION::BUFFERCONF::GAINONOFF);
    cfg.gainauto        = node.get<int>(IMACQUISITION::BUFFERCONF::GAINAUTO);
    cfg.whitebalance    = node.get<int>(IMACQUISITION::BUFFERCONF::WHITEBALANCE);
    cfg.hwtrigger       = node.get<int>(IMACQUISITION::BUFFERCONF::HWTRIGGER);
    cfg.hwtriggerparam  = node.get<int>(IMACQUISITION::BUFFERCONF::HWTRIGGERPARAM);
    cfg.hwtriggersrc    = node.get<int>(IMACQUISITION::BUFFERCONF::HWTRIGGERSOURCE);

    return cfg;
}

EncoderQualityConfig SettingsIAC::getBufferConf(int camid)
{
    EncoderQualityConfig cfg;
    std::stringstream    cid;

    cfg.camid = -1;
    cid << camid;

    // Find the subtree having the right CAMID value
    BOOST_FOREACH (boost::property_tree::ptree::value_type& v, _ptree.get_child("IMACQUISITION"))
    {
        if (v.first == "BUFFER")
        {
            BOOST_FOREACH (boost::property_tree::ptree::value_type& w, v.second)
            {
                std::string snd = w.second.data();
                if (w.first == "CAMID" && snd == cid.str())
                {
                    return setFromNode(v.second);
                }
            }
        }
    }
    return cfg;
}

void SettingsIAC::loadNewSettings()
{
    for (const auto& [_, videoStreamTree] : _ptree.get_child("video_streams"))
    {
        VideoStream stream;

        const auto& cameraTree = videoStreamTree.get_child("camera");

        stream.camera.serial = cameraTree.get<std::string>("serial");
        stream.camera.id     = cameraTree.get<int>("id");
        stream.camera.width  = cameraTree.get<int>("width");
        stream.camera.height = cameraTree.get<int>("height");

        const auto& triggerTree = cameraTree.get_child("trigger");

        const auto triggerType = triggerTree.get<std::string>("type");
        if (triggerType == "hardware")
        {
            VideoStream::Camera::HardwareTrigger trigger;
            trigger.source          = triggerTree.get<int>("source");
            trigger.framesPerSecond = triggerTree.get<int>("frames_per_second");
            stream.camera.trigger   = trigger;
        }
        else if (triggerType == "software")
        {
            VideoStream::Camera::SoftwareTrigger trigger;
            trigger.framesPerSecond = triggerTree.get<int>("frames_per_second");
            stream.camera.trigger   = trigger;
        }
        else
        {
            std::ostringstream msg;
            msg << "Invalid camera trigger type: " << triggerType;
            throw std::runtime_error(msg.str());
        }

        stream.camera.buffersize = cameraTree.get_optional<int>("buffersize");

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

        stream.camera.brightness = parseAutoIntParam(cameraTree, "brightness");
        stream.camera.exposure   = parseAutoIntParam(cameraTree, "exposure");
        stream.camera.shutter    = parseAutoIntParam(cameraTree, "shutter");
        stream.camera.gain       = parseAutoIntParam(cameraTree, "gain");

        stream.camera.whitebalance = cameraTree.get_optional<bool>("whitebalance");

        stream.framesPerVideoFile = videoStreamTree.get<int>("frames_per_video_file");

        for (const auto& [key, value] : videoStreamTree.get_child("encoder"))
        {
            if (key == "name")
            {
                stream.encoder.name = value.get_value<std::string>();
            }
            else
            {
                stream.encoder.options.emplace(key, value.get_value<std::string>());
            }
        }

        _videoStreams.push_back(stream);
    }

    _logDirectory = _ptree.get_value<std::string>("log_directory");
    _tmpPath      = _ptree.get_value<std::string>("tmpPath");
    _outDirectory = _ptree.get_value<std::string>("outDirectory");
}

const std::vector<SettingsIAC::VideoStream>& SettingsIAC::videoStreams() const
{
    return _videoStreams;
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
