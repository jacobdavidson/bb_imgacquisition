// SPDX-License-Identifier: GPL-3.0-or-later

#include "BaslerCamera.hpp"

#include <chrono>
#include <optional>

#include <opencv2/core/mat.hpp>

#include "util/format.hpp"
#include "util/log.hpp"
#include "util/type_traits.hpp"
#include "GrayscaleImage.hpp"

#include <pylon/usb/BaslerUsbInstantCamera.h>

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

        config.params.offset_x = camera.OffsetX();
        config.params.offset_y = camera.OffsetY();
        config.params.width    = camera.Width();
        config.params.height   = camera.Height();

        config.params.trigger = SoftwareTrigger{static_cast<float>(camera.ResultingFrameRate())};

        config.params.blacklevel = Parameter_Manual<float>(camera.BlackLevel());

        if (camera.ExposureAuto() == ExposureAuto_Off)
        {
            config.params.exposure = Parameter_Manual<float>(camera.ExposureTime());
        }
        else
        {
            config.params.exposure = Parameter_Auto{};
        }

        if (camera.GainAuto() == GainAuto_Off)
        {
            config.params.gain = Parameter_Manual<float>(camera.Gain());
        }
        else
        {
            config.params.gain = Parameter_Auto{};
        }

        config.offset_x = 0;
        config.offset_y = 0;
        config.width    = config.params.width;
        config.height   = config.params.height;

        cameraConfigs.push_back(config);
    }

    return cameraConfigs;
}

BaslerCamera::BaslerCamera(Config config, ImageStream imageStream)
: Camera(config, imageStream)
{
    initCamera();
    startCapture();
}

void BaslerCamera::initCamera()
{
    using namespace std::chrono_literals;
    using namespace Pylon;
    using namespace Basler_UsbCameraParams;

    if (_config.serial.empty())
    {
        throw std::runtime_error(fmt::format("{}: Camera serial empty", _imageStream.id));
    }

    try
    {
        auto& factory = CTlFactory::GetInstance();

        // Get all attached devices and return -1 if no device is found.
        DeviceInfoList_t devices;

        if (auto r = factory.EnumerateDevices(devices); r < 0)
        {
            throw std::runtime_error(
                fmt::format("{}: Could not enumerate cameras", _imageStream.id));
        }

        std::optional<CDeviceInfo> camDeviceInfo;
        for (std::size_t i = 0; i < devices.size(); i++)
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
                fmt::format("{}: No camera matching serial: {}", _imageStream.id, _config.serial));
        }

        // NOTE: Basler camera models appear to use the naming convention
        //       (series)(...)(interface)(mono/color)[suffix]

        const auto cameraModel = std::string(camDeviceInfo->GetModelName());
        logInfo("{}: Camera model: {}", _imageStream.id, cameraModel);

        if (cameraModel.size() < 2)
        {
            throw std::runtime_error(
                fmt::format("{}: Cannot determine camera series from model name {}",
                            _imageStream.id,
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

        if (cameraSeries == "ace" && cameraInterface == BaslerUsbDeviceClass)
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
                _imageStream.id,
                _camera.SensorWidth(),
                _camera.SensorHeight());

        _camera.OffsetX = _config.params.offset_x;
        _camera.OffsetY = _config.params.offset_y;
        _camera.Width   = _config.params.width;
        _camera.Height  = _config.params.height;

        logInfo("{}: Camera region of interest: ({},{}),{}x{}",
                _imageStream.id,
                _camera.OffsetX(),
                _camera.OffsetY(),
                _camera.Width(),
                _camera.Height());

        if (!(_camera.OffsetX() == _config.params.offset_x &&
              _camera.OffsetY() == _config.params.offset_y &&
              _camera.Width() == _config.params.width &&
              _camera.Height() == _config.params.height))
        {
            throw std::runtime_error(
                fmt::format("{}: Could not set camera region of intereset to ({},{}),{}x{}",
                            _imageStream.id,
                            _config.params.offset_x,
                            _config.params.offset_y,
                            _config.params.width,
                            _config.params.height));
        }

        std::visit(
            [this](auto&& trigger) {
                const auto mapTriggerSource = [](int source) -> std::optional<TriggerSourceEnums> {
                    switch (source)
                    {
                    case 1:
                        return TriggerSource_Line1;
                    case 2:
                        return TriggerSource_Line2;
                    case 3:
                        return TriggerSource_Line3;
                    case 4:
                        return TriggerSource_Line4;
                    default:
                        return std::nullopt;
                    }
                };

                using Trigger = std::decay_t<decltype(trigger)>;

                if constexpr (std::is_same_v<Trigger, HardwareTrigger>)
                {
                    auto triggerSource = mapTriggerSource(trigger.source);
                    if (triggerSource)
                    {
                        _camera.TriggerSelector = TriggerSelector_FrameStart;
                        _camera.TriggerSource   = *triggerSource;
                        _camera.TriggerMode     = TriggerMode_On;

                        _camera.AcquisitionFrameRateEnable = 0;
                    }
                    else
                    {
                        throw std::runtime_error("Invalid camera hardware trigger source");
                    }
                }
                else if constexpr (std::is_same_v<Trigger, SoftwareTrigger>)
                {
                    _camera.TriggerSelector = TriggerSelector_FrameStart;
                    _camera.TriggerMode     = TriggerMode_Off;

                    _camera.AcquisitionFrameRateEnable = 1;
                    _camera.AcquisitionFrameRate       = trigger.framesPerSecond;
                }
                else
                {
                    static_assert(false_type<Trigger>::value);
                }
            },
            _config.params.trigger);

        if (_config.params.blacklevel)
        {
            std::visit(
                [this](auto&& value) {
                    using T = std::decay_t<decltype(value)>;

                    if constexpr (std::is_same_v<T, Parameter_Auto>)
                        logWarning("{}: Automatic black level not supported on Basler cameras",
                                   _imageStream.id);
                    else if constexpr (std::is_same_v<T, Parameter_Manual<float>>)
                        _camera.BlackLevel = value;
                    else
                        static_assert(false_type<T>::value);
                },
                *_config.params.blacklevel);
        }

        if (_config.params.exposure && _config.params.gain)
        {
            const auto autoExposure = std::holds_alternative<Parameter_Auto>(
                *_config.params.exposure);
            const auto autoGain = std::holds_alternative<Parameter_Auto>(*_config.params.exposure);

            if (autoExposure && autoGain)
            {
                _camera.ExposureAuto = ExposureAuto_Continuous;
                _camera.GainAuto     = GainAuto_Continuous;
            }
            else if (!autoExposure && !autoGain)
            {
                _camera.ExposureAuto = ExposureAuto_Off;
                _camera.GainAuto     = GainAuto_Off;

                _camera.ExposureTime = std::get<Parameter_Manual<float>>(*_config.params.exposure);
                _camera.Gain         = std::get<Parameter_Manual<float>>(*_config.params.gain);
            }
            else
            {
                throw std::runtime_error(
                    fmt::format("{}: Automatic gain enables automatic exposure and vice versa on "
                                "Basler cameras",
                                _imageStream.id));
            }
        }
        else if (_config.params.exposure)
        {
            _camera.ExposureAuto = ExposureAuto_Off;
            _camera.GainAuto     = GainAuto_Off;

            _camera.ExposureTime = std::get<Parameter_Manual<float>>(*_config.params.exposure);
        }
        else if (_config.params.gain)
        {
            _camera.ExposureAuto = ExposureAuto_Off;
            _camera.GainAuto     = GainAuto_Off;

            _camera.Gain = std::get<Parameter_Manual<float>>(*_config.params.gain);
        }
    }
    catch (GenericException e)
    {
        throw std::runtime_error(
            fmt::format("{}: Failed to initialize camera: {}", _imageStream.id, e.what()));
    }
}

void BaslerCamera::startCapture()
{
    using namespace Pylon;

    try
    {
        _camera.StartGrabbing();
    }
    catch (GenericException e)
    {
        throw std::runtime_error(fmt::format("{}: Failed to start capturing with camera: {}",
                                             _imageStream.id,
                                             e.what()));
    }
}

void BaslerCamera::run()
{
    using namespace std::chrono_literals;
    using namespace Pylon;
    using namespace Basler_UsbCameraParams;

    std::uint64_t lastImageNumber = 0;

    // The camera timestamp will be used to get a more accurate idea of when the image was taken.
    // Software hangups (e.g. short CPU spikes) can thus be mitigated.
    auto currCameraTime      = std::chrono::system_clock::time_point{};
    auto lastCameraTimestamp = 0ns;

    std::uint8_t* p_image;
    uint          img_width, img_height;

    for (std::size_t loopCount = 0; !isInterruptionRequested(); loopCount += 1)
    {
        if (_camera.IsGrabbing())
        {
            const auto begin = std::chrono::steady_clock::now();

            if (std::holds_alternative<HardwareTrigger>(_config.params.trigger))
            {
                // Watchdog demands heartbeats at an interval of at most 60 seconds
                _camera.RetrieveResult(30 * 1000, _grabbed, TimeoutHandling_Return);
                if (!_grabbed)
                    continue;
            }
            else if (std::holds_alternative<SoftwareTrigger>(_config.params.trigger))
            {
                _camera.RetrieveResult(1000, _grabbed, TimeoutHandling_Return);
                if (!_grabbed)
                {
                    logCritical("{}: Failed to grab camera image: {}",
                                _imageStream.id,
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
            p_image    = static_cast<std::uint8_t*>(_grabbed->GetBuffer());

            if (!(img_width == _config.params.width && img_height == _config.params.height))
            {
                throw std::runtime_error(
                    fmt::format("{}: Camera captured image of incorrect size: {}x{}",
                                _imageStream.id,
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
                logWarning("{}: Camera lost frame: This frame: #{} last frame: #{} timestamp: {}",
                           _imageStream.id,
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
                        _imageStream.id,
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
            const std::int64_t duration =
                std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
            if (duration > 2 * (1000000 / 6))
            {
                logWarning("{}: Processing time too long: {}", _imageStream.id, duration);
            }
        }
        else
        {
            throw std::runtime_error(fmt::format("{}: Camera stopped capturing", _imageStream.id));
        }

        auto cvImage = cv::Mat{cv::Size{static_cast<int>(img_width), static_cast<int>(img_height)},
                               CV_8UC1,
                               p_image};
        if (!(img_width == _config.width && img_height == _config.height))
        {
            cvImage = cvImage(
                cv::Rect{_config.offset_x, _config.offset_y, _config.width, _config.height});
        }

        auto capturedImage = GrayscaleImage(cvImage.cols, cvImage.rows, currCameraTime);
        std::memcpy(&capturedImage.data[0], cvImage.data, cvImage.cols * cvImage.rows);

        _imageStream.push(capturedImage);
        emit imageCaptured(capturedImage);
    }

    return;
}
