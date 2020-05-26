// SPDX-License-Identifier: GPL-3.0-or-later

#include "BaslerCamera.h"

#include <chrono>
#include <optional>

#include "Watchdog.h"
#include "format.h"
#include "log.h"
#include "GrayscaleImage.h"

#include <pylon/usb/BaslerUsbInstantCamera.h>

template<typename T>
struct dependent_false : std::false_type
{
};

auto BaslerCamera::getAvailable() -> std::vector<Config>
{
    using namespace Pylon;
    using namespace Basler_UsbCameraParams;

    auto  pylon   = PylonAutoInitTerm{};
    auto& factory = CTlFactory::GetInstance();

    auto deviceInfos = DeviceInfoList_t{};
    if (auto r = factory.EnumerateDevices(deviceInfos); r < 0)
    {
        return {};
    }

    std::vector<Config> cameraConfigs;
    for (const auto deviceInfo : deviceInfos)
    {
        Config config;
        config.backend = "basler";
        config.serial  = deviceInfo.GetSerialNumber();

        if (deviceInfo.GetDeviceClass() != BaslerUsbDeviceClass)
        {
            continue;
        }

        auto camera = CBaslerUsbInstantCamera(factory.CreateDevice(deviceInfo));
        camera.Open();

        config.offset_x = camera.OffsetX();
        config.offset_y = camera.OffsetY();
        config.width    = camera.Width();
        config.height   = camera.Height();

        if (camera.TriggerMode() == TriggerMode_On &&
            TriggerSource_Line1 <= camera.TriggerSource() &&
            camera.TriggerSource() <= TriggerSource_Line4)
        {
            config.trigger = Config::HardwareTrigger{camera.TriggerSource()};
        }
        else
        {
            config.trigger = Config::SoftwareTrigger{
                static_cast<float>(camera.ResultingFrameRate())};
        }

        config.blacklevel = Config::Parameter_Manual<float>(camera.BlackLevel());

        if (camera.ExposureAuto() == ExposureAuto_Off)
        {
            config.exposure = Config::Parameter_Manual<float>(camera.ExposureTime());
        }
        else
        {
            config.exposure = Config::Parameter_Auto{};
        }

        if (camera.GainAuto() == GainAuto_Off)
        {
            config.gain = Config::Parameter_Manual<float>(camera.Gain());
        }
        else
        {
            config.gain = Config::Parameter_Auto{};
        }

        cameraConfigs.push_back(config);
    }

    return cameraConfigs;
}

BaslerCamera::BaslerCamera(Config config, VideoStream videoStream, Watchdog* watchdog)
: Camera(config, videoStream, watchdog)
{
    initCamera();
    startCapture();
}

void BaslerCamera::initCamera()
{
    using namespace std::chrono_literals;

    if (_config.serial.empty())
    {
        throw std::runtime_error(fmt::format("{}: Camera serial empty", _videoStream.id));
    }

    try
    {
        auto& factory = Pylon::CTlFactory::GetInstance();

        // Get all attached devices and return -1 if no device is found.
        Pylon::DeviceInfoList_t devices;

        if (auto r = factory.EnumerateDevices(devices); r < 0)
        {
            throw std::runtime_error(
                fmt::format("{}: Could not enumerate cameras", _videoStream.id));
        }

        std::optional<Pylon::CDeviceInfo> camDeviceInfo;
        for (size_t i = 0; i < devices.size(); i++)
        {
            if (devices[i].GetSerialNumber() == _config.serial.c_str())
            {
                camDeviceInfo = devices[i];
                break;
            }
        }

        if (!camDeviceInfo)
        {
            throw std::runtime_error(
                fmt::format("{}: No camera matching serial: {}", _videoStream.id, _config.serial));
        }

        // NOTE: Basler camera models appear to use the naming convention
        //       (series)(...)(interface)(mono/color)[suffix]

        const auto cameraModel = std::string(camDeviceInfo->GetModelName());
        logInfo("{}: Camera model: {}", _videoStream.id, cameraModel);

        if (cameraModel.size() < 2)
        {
            throw std::runtime_error(
                fmt::format("{}: Cannot determine camera series from model name {}",
                            _videoStream.id,
                            cameraModel));
        }

        const auto cameraSeries = [](auto code) {
            if (code == "ac")
                return "ace";
            else if (code == "da")
                return "dart";
            else if (code == "pu")
                return "pulse";
            else
                return "unknown";
        }(cameraModel.substr(0, 2));

        const auto cameraInterface = camDeviceInfo->GetDeviceClass();

        // clang-format off
        //
        // Pylon SDK states the following about camera timestamp support:
        //
        // Camera Model                         | Timestamp Tick Frequency
        //
        // All ace USB 3.0 camera models        | 1 GHz (= 1 000 000 000 ticks per second, 1 tick = 1 ns)
        // All ace GigE camera models           | 125 MHz (= 125 000 000  ticks per second, 1 tick = 8 ns) or 1 GHz (= 1 000 000 000 ticks per second, 1 tick = 1 ns)
        // All dart BCON for LVDS camera models | Timestamp Latch feature not supported
        // All dart BCON for MIPI camera models | Timestamp Latch feature not supported
        // All dart USB 3.0 camera models       | Timestamp Latch feature not supported
        // All pulse USB 3.0 camera models      | Timestamp Latch feature not supported
        //
        // clang-format on

        if (cameraSeries == "ace" && cameraInterface == Pylon::BaslerUsbDeviceClass)
        {
            _nsPerTick = 1ns;
        }
        else
        {
            throw std::runtime_error(fmt::format("{}: Only ace USB 3.0 cameras supported"));
        }

        _camera.Attach(factory.CreateDevice(*camDeviceInfo));
        _camera.Open();

        logInfo("{}: Camera resolution: {}x{}",
                _videoStream.id,
                _camera.SensorWidth(),
                _camera.SensorHeight());

        _camera.OffsetX = _config.offset_x;
        _camera.OffsetY = _config.offset_y;
        _camera.Width   = _config.width;
        _camera.Height  = _config.height;

        logInfo("{}: Camera region of interest: ({},{}),{}x{}",
                _videoStream.id,
                _camera.OffsetX(),
                _camera.OffsetY(),
                _camera.Width(),
                _camera.Height());

        if (!(_camera.OffsetX() == _config.offset_x && _camera.OffsetY() == _config.offset_y &&
              _camera.Width() == _config.width && _camera.Height() == _config.height))
        {
            throw std::runtime_error(
                fmt::format("{}: Could not set camera region of intereset to ({},{}),{}x{}",
                            _videoStream.id,
                            _config.offset_x,
                            _config.offset_y,
                            _config.width,
                            _config.height));
        }

        std::visit(
            [this](auto&& trigger) {
                const auto mapTriggerSource =
                    [](int source) -> std::optional<Basler_UsbCameraParams::TriggerSourceEnums> {
                    switch (source)
                    {
                    case 1:
                        return Basler_UsbCameraParams::TriggerSource_Line1;
                    case 2:
                        return Basler_UsbCameraParams::TriggerSource_Line2;
                    case 3:
                        return Basler_UsbCameraParams::TriggerSource_Line3;
                    case 4:
                        return Basler_UsbCameraParams::TriggerSource_Line4;
                    default:
                        return std::nullopt;
                    }
                };

                using Trigger = std::decay_t<decltype(trigger)>;

                if constexpr (std::is_same_v<Trigger, Config::HardwareTrigger>)
                {
                    auto triggerSource = mapTriggerSource(trigger.source);
                    if (triggerSource)
                    {
                        _camera.TriggerSelector =
                            Basler_UsbCameraParams::TriggerSelector_FrameStart;
                        _camera.TriggerSource = *triggerSource;
                        _camera.TriggerMode   = Basler_UsbCameraParams::TriggerMode_On;

                        _camera.AcquisitionFrameRateEnable = 0;
                    }
                    else
                    {
                        throw std::runtime_error("Invalid camera hardware trigger source");
                    }
                }
                else if constexpr (std::is_same_v<Trigger, Config::SoftwareTrigger>)
                {
                    _camera.TriggerSelector = Basler_UsbCameraParams::TriggerSelector_FrameStart;
                    _camera.TriggerMode     = Basler_UsbCameraParams::TriggerMode_Off;

                    _camera.AcquisitionFrameRateEnable = 1;
                    _camera.AcquisitionFrameRate       = trigger.framesPerSecond;
                }
                else
                {
                    static_assert(dependent_false<Trigger>::value);
                }
            },
            _config.trigger);

        if (_config.blacklevel)
        {
            std::visit(
                [this](auto&& value) {
                    using T = std::decay_t<decltype(value)>;

                    if constexpr (std::is_same_v<T, Config::Parameter_Auto>)
                        logWarning("{}: Automatic black level not supported on Basler cameras",
                                   _videoStream.id);
                    else if constexpr (std::is_same_v<T, Config::Parameter_Manual<float>>)
                        _camera.BlackLevel = value;
                    else
                        static_assert(dependent_false<T>::value);
                },
                *_config.blacklevel);
        }

        if (_config.exposure && _config.gain)
        {
            const auto autoExposure = std::holds_alternative<Config::Parameter_Auto>(
                *_config.exposure);
            const auto autoGain = std::holds_alternative<Config::Parameter_Auto>(
                *_config.exposure);

            if (autoExposure && autoGain)
            {
                _camera.ExposureAuto = Basler_UsbCameraParams::ExposureAuto_Continuous;
                _camera.GainAuto     = Basler_UsbCameraParams::GainAuto_Continuous;
            }
            else if (!autoExposure && !autoGain)
            {
                _camera.ExposureAuto = Basler_UsbCameraParams::ExposureAuto_Off;
                _camera.GainAuto     = Basler_UsbCameraParams::GainAuto_Off;

                _camera.ExposureTime = std::get<Config::Parameter_Manual<float>>(
                    *_config.exposure);
                _camera.Gain = std::get<Config::Parameter_Manual<float>>(*_config.gain);
            }
            else
            {
                throw std::runtime_error(
                    fmt::format("{}: Automatic gain enables automatic exposure and vice versa on "
                                "Basler cameras",
                                _videoStream.id));
            }
        }
        else if (_config.exposure)
        {
            _camera.ExposureAuto = Basler_UsbCameraParams::ExposureAuto_Off;
            _camera.GainAuto     = Basler_UsbCameraParams::GainAuto_Off;

            _camera.ExposureTime = std::get<Config::Parameter_Manual<float>>(*_config.exposure);
        }
        else if (_config.gain)
        {
            _camera.ExposureAuto = Basler_UsbCameraParams::ExposureAuto_Off;
            _camera.GainAuto     = Basler_UsbCameraParams::GainAuto_Off;

            _camera.Gain = std::get<Config::Parameter_Manual<float>>(*_config.gain);
        }
    }
    catch (Pylon::GenericException e)
    {
        throw std::runtime_error(
            fmt::format("{}: Failed to initialize camera: {}", _videoStream.id, e.what()));
    }
}

void BaslerCamera::startCapture()
{
    try
    {
        _camera.StartGrabbing();
    }
    catch (Pylon::GenericException e)
    {
        throw std::runtime_error(fmt::format("{}: Failed to start capturing with camera: {}",
                                             _videoStream.id,
                                             e.what()));
    }
}

void BaslerCamera::run()
{
    using namespace std::chrono_literals;

    const unsigned int vwidth  = static_cast<unsigned int>(_config.width);
    const unsigned int vheight = static_cast<unsigned int>(_config.height);

    uint64_t lastImageNumber = 0;

    // The camera timestamp will be used to get a more accurate idea of when the image was taken.
    // Software hangups (e.g. short CPU spikes) can thus be mitigated.
    auto currCameraTime      = std::chrono::system_clock::time_point{};
    auto lastCameraTimestamp = 0ns;

    uint8_t* p_image;
    uint     img_width, img_height;

    for (size_t loopCount = 0; !isInterruptionRequested(); loopCount += 1)
    {
        _watchdog->pulse();

        if (_camera.IsGrabbing())
        {
            try
            {
                const auto begin = std::chrono::steady_clock::now();

                if (std::holds_alternative<Config::HardwareTrigger>(_config.trigger))
                {
                    // Watchdog demands heartbeats at an interval of at most 60 seconds
                    _camera.RetrieveResult(30 * 1000, _grabbed, Pylon::TimeoutHandling_Return);
                    if (!_grabbed)
                        continue;
                }
                else if (std::holds_alternative<Config::SoftwareTrigger>(_config.trigger))
                {
                    _camera.RetrieveResult(1000, _grabbed, Pylon::TimeoutHandling_Return);
                    if (!_grabbed)
                    {
                        logCritical("{}: Failed to grab camera image: {}",
                                    _videoStream.id,
                                    _grabbed->GetErrorDescription());
                        continue;
                    }
                }
                else
                {
                    throw std::logic_error("Not implemented");
                }

                const auto end = std::chrono::steady_clock::now();

                img_width  = _grabbed->GetWidth();
                img_height = _grabbed->GetHeight();
                p_image    = static_cast<uint8_t*>(_grabbed->GetBuffer());

                if (!(img_width == _config.width && img_height == _config.height))
                {
                    throw std::logic_error(
                        fmt::format("{}: Camera captured image of incorrect size: {}x{}",
                                    _videoStream.id,
                                    img_width,
                                    img_height));
                }

                const auto currImageNumber = _grabbed->GetImageNumber();

                const auto currWallClockTime = std::chrono::system_clock::now();
                // NOTE: Camera time starts when the camera is powered on.
                //       It thus resets whenever power is turned off and on again.
                const auto currCameraTimestamp = _nsPerTick * _grabbed->GetTimeStamp();

                // Image sequence sanity check.
                if (lastImageNumber != 0 && currImageNumber != lastImageNumber + 1)
                {
                    logWarning(
                        "{}: Camera lost frame: This frame: #{} last frame: #{} timestamp: {}",
                        _videoStream.id,
                        currImageNumber,
                        lastImageNumber,
                        currWallClockTime);
                }
                lastImageNumber = currImageNumber;

                if (lastCameraTimestamp == 0ns || lastCameraTimestamp > currCameraTimestamp)
                {
                    if (lastCameraTimestamp != 0ns && currCameraTime > currWallClockTime &&
                        loopCount > 0)
                    {
                        logWarning(
                            "{}: Camera clock faster than wall time. Last camera time: {:e.6} "
                            "wall "
                            "clock: {:e.6}",
                            _videoStream.id,
                            currCameraTime,
                            currWallClockTime);
                    }
                    currCameraTime = currWallClockTime;
                }
                else
                {
                    const auto elapsed = currCameraTimestamp - lastCameraTimestamp;
                    assert(elapsed >= 0ns);
                    currCameraTime += elapsed;
                }
                lastCameraTimestamp = currCameraTimestamp;

                // Check if processing a frame took longer than X seconds. If
                // so, log the event.
                // TODO: Why this number: 2 * (1000000 / 6)
                const int64_t duration =
                    std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
                if (duration > 2 * (1000000 / 6))
                {
                    logWarning("{}: Processing time too long: {}", _videoStream.id, duration);
                }
            }
            catch (Pylon::GenericException e) // could not retrieve grab result
            {
                logWarning("{}: Camera error: {}", _videoStream.id, e.what());

                // TODO: Terminate application if the problem persists and we get this error
                // repeatedly
            }
        }
        else
        {
            throw std::runtime_error(fmt::format("{}: Camera stopped capturing", _videoStream.id));
        }

        auto buf = GrayscaleImage(_config.width, _config.height, currCameraTime);
        memcpy(&buf.data[0], p_image, _config.width * _config.height);

        _videoStream.push(buf);
        emit imageCaptured(_videoStream.id, buf);
    }

    return;
}
