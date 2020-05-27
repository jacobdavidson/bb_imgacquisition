// SPDX-License-Identifier: GPL-3.0-or-later

#include "Settings.hpp"

#include <fstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <QStandardPaths>
#include <QDir>

#include "util/format.hpp"
#include "util/log.hpp"
#include "util/type_traits.hpp"

#if defined(USE_BASLER) && USE_BASLER
    #include "camera/BaslerCamera.hpp"
#endif

template<typename T>
auto parseParam(const boost::property_tree::ptree& tree, const std::string& name)
    -> std::optional<Camera::Config::Parameter<T>>
{
    if (auto strValue = tree.get_optional<std::string>(name); strValue)
    {
        if (*strValue == "auto")
        {
            return Camera::Config::Parameter_Auto{};
        }
        else if (auto value = tree.get_optional<T>(name); value)
        {
            return Camera::Config::Parameter_Manual<T>{*value};
        }

        throw std::runtime_error(
            fmt::format("Invalid value for camera parameter \'{}\': Expected \"auto\" or "
                        "appropriately typed value",
                        name));
    }

    return {};
};

boost::property_tree::ptree detectSettings()
{
    auto tree = boost::property_tree::ptree{};

    tree.put("tmp_dir", "data/tmp");
    tree.put("out_dir", "data/out");

    auto& videoEncoders = tree.put_child("encoders", {});
    videoEncoders.put("h265_sw_0", "libx265");

    auto& imageStreams = tree.put_child("streams", {});

    std::size_t camIndex = 0;

    auto addImageStreamForCameraConfig = [&camIndex, &imageStreams](Camera::Config config) {
        auto& imageStreamTree = imageStreams.put_child(fmt::format("cam-{}", camIndex++), {});

        auto& cameraTree = imageStreamTree.put_child("camera", {});
        cameraTree.put("backend", config.backend);
        cameraTree.put("serial", config.serial);

        cameraTree.put("offset_x", config.offset_x);
        cameraTree.put("offset_y", config.offset_y);
        cameraTree.put("width", config.width);
        cameraTree.put("height", config.height);

        auto& triggerTree = cameraTree.put_child("trigger", {});

        std::visit(
            [&](auto&& value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, Camera::Config::HardwareTrigger>)
                {
                    triggerTree.put("type", "hardware");
                    triggerTree.put("source", value.source);

                    imageStreamTree.put("frames_per_second", "FRAMES_PER_SECOND");
                }
                else if constexpr (std::is_same_v<T, Camera::Config::SoftwareTrigger>)
                {
                    triggerTree.put("type", "software");
                    triggerTree.put("frames_per_second", value.framesPerSecond);

                    imageStreamTree.put("frames_per_second", value.framesPerSecond);
                }
                else
                    static_assert(false_type<T>::value);
            },
            config.trigger);

        if (config.blacklevel)
        {
            std::visit(
                [&](auto&& value) {
                    using T = std::decay_t<decltype(value)>;

                    if constexpr (std::is_same_v<T, Camera::Config::Parameter_Auto>)
                        cameraTree.put("blacklevel", "auto");
                    else if constexpr (std::is_same_v<T, Camera::Config::Parameter_Manual<float>>)
                        cameraTree.put("blacklevel", value);
                    else
                        static_assert(false_type<T>::value);
                },
                *config.blacklevel);
        }

        if (config.exposure)
        {
            std::visit(
                [&](auto&& value) {
                    using T = std::decay_t<decltype(value)>;

                    if constexpr (std::is_same_v<T, Camera::Config::Parameter_Auto>)
                        cameraTree.put("exposure", "auto");
                    else if constexpr (std::is_same_v<T, Camera::Config::Parameter_Manual<float>>)
                        cameraTree.put("exposure", value);
                    else
                        static_assert(false_type<T>::value);
                },
                *config.exposure);
        }

        if (config.gain)
        {
            std::visit(
                [&](auto&& value) {
                    using T = std::decay_t<decltype(value)>;

                    if constexpr (std::is_same_v<T, Camera::Config::Parameter_Auto>)
                        cameraTree.put("gain", "auto");
                    else if constexpr (std::is_same_v<T, Camera::Config::Parameter_Manual<float>>)
                        cameraTree.put("gain", value);
                    else
                        static_assert(false_type<T>::value);
                },
                *config.gain);
        }

        imageStreamTree.put("frames_per_file", 500);

        auto& encoderTree = imageStreamTree.put_child("encoder", {});

        encoderTree.put("id", "h265_sw_0");

        auto& encoderOptions = encoderTree.put_child("options", {});
        encoderOptions.put("preset", "5");
        encoderOptions.put("x265-params", "log-level=error");
    };

#if defined(USE_BASLER) && USE_BASLER
    for (auto config : BaslerCamera::getAvailable())
    {
        addImageStreamForCameraConfig(config);
    }
#endif

    return tree;
}

const Settings& Settings::instance()
{
    static const Settings instance;
    return instance;
}

Settings::Settings()
{
    const auto configLocation = QDir(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!configLocation.exists())
    {
        configLocation.mkpath(".");
    }

    const auto configFilename = configLocation.absoluteFilePath("config.json").toStdString();

    std::fstream configFile;
    configFile.open(configFilename, std::ios::in);
    if (!configFile.good())
    {
        configFile.open(configFilename, std::ios::out | std::ios::trunc);
        configFile.imbue(std::locale::classic());
        boost::property_tree::write_json(configFile, detectSettings());
        logInfo("New settings file created: Edit it manually and run application again: {}",
                configFilename);
        std::exit(0);
    }

    configFile.imbue(std::locale::classic());
    auto tree = boost::property_tree::ptree{};
    boost::property_tree::read_json(configFile, tree);

    for (const auto& [imageStreamId, imageStreamTree] : tree.get_child("streams"))
    {
        ImageStream stream;

        stream.id = imageStreamId;

        const auto& cameraTree = imageStreamTree.get_child("camera");

        stream.camera.backend = cameraTree.get<std::string>("backend");
        stream.camera.serial  = cameraTree.get<std::string>("serial");

        stream.camera.offset_x = cameraTree.get<int>("offset_x");
        stream.camera.offset_y = cameraTree.get<int>("offset_y");
        stream.camera.width    = cameraTree.get<int>("width");
        stream.camera.height   = cameraTree.get<int>("height");

        const auto& triggerTree = cameraTree.get_child("trigger");

        const auto triggerType = triggerTree.get<std::string>("type");
        if (triggerType == "hardware")
        {
            auto trigger          = Camera::Config::HardwareTrigger{};
            trigger.source        = triggerTree.get<int>("source");
            stream.camera.trigger = trigger;
        }
        else if (triggerType == "software")
        {
            auto trigger            = Camera::Config::SoftwareTrigger{};
            trigger.framesPerSecond = triggerTree.get<float>("frames_per_second");
            stream.camera.trigger   = trigger;
        }
        else
        {
            throw std::runtime_error(fmt::format("Invalid camera trigger type: {}", triggerType));
        }

        stream.camera.buffer_size      = cameraTree.get_optional<int>("buffer_size");
        stream.camera.throughput_limit = cameraTree.get_optional<int>("throughput_limit");

        stream.camera.blacklevel = parseParam<float>(cameraTree, "blacklevel");
        stream.camera.exposure   = parseParam<float>(cameraTree, "exposure");
        stream.camera.gain       = parseParam<float>(cameraTree, "gain");

        stream.framesPerSecond = imageStreamTree.get<float>("frames_per_second");
        stream.framesPerFile   = imageStreamTree.get<std::size_t>("frames_per_file");

        const auto& encoderTree = imageStreamTree.get_child("encoder");
        stream.encoder.id       = encoderTree.get<std::string>("id");

        for (const auto& [key, value] : encoderTree.get_child("options"))
        {
            stream.encoder.options.emplace(key, value.get_value<std::string>());
        }

        _imageStreams.push_back(stream);
    }

    for (auto& [id, name] : tree.get_child("encoders"))
    {
        _videoEncoders.emplace(id, name.get_value<std::string>());
    }

    _temporaryDirectory = tree.get<std::string>("tmp_dir");
    _outputDirectory    = tree.get<std::string>("out_dir");
}

const std::vector<Settings::ImageStream>& Settings::imageStreams() const
{
    return _imageStreams;
}

const std::map<std::string, std::string>& Settings::videoEncoders() const
{
    return _videoEncoders;
}

const std::string Settings::temporaryDirectory() const
{
    return _temporaryDirectory;
}

const std::string Settings::outputDirectory() const
{
    return _outputDirectory;
}
