// SPDX-License-Identifier: GPL-3.0-or-later

#include "BaslerCamThread.h"

#include <cstdlib>
#include <iostream>

#include <QDebug>
#include <qdir.h> //QT stuff
#include <qtextstream.h>

#include "Watchdog.h"
#include "settings/Settings.h"
#include "settings/utility.h"
#include "GrayscaleImage.h"
#include "log.h"

#include <sstream> //stringstreams

#include <array>
#include <ctime> //get time
#include <time.h>

#include <opencv2/opencv.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time.hpp>

template<typename T>
struct dependent_false : std::false_type
{
};

BaslerCamThread::BaslerCamThread(Config config, VideoStream videoStream, Watchdog* watchdog)
: CamThread(config, videoStream, watchdog)
{
    initCamera();
    startCapture();
}

void BaslerCamThread::initCamera()
{
    if (_config.serial.empty())
    {
        throw std::runtime_error(fmt::format("{}: Camera serial empty", _videoStream.id));
    }

    try
    {
        // Get the transport layer factory.
        auto& tlFactory = Pylon::CTlFactory::GetInstance();

        // Get all attached devices and return -1 if no device is found.
        Pylon::DeviceInfoList_t devices;

        if (auto r = tlFactory.EnumerateDevices(devices); r < 0)
        {
            throw std::runtime_error(
                fmt::format("{}: Could not enumerate cameras", _videoStream.id));
        }

        bool camFound = false;
        for (size_t i = 0; i < devices.size(); i++)
        {
            if (devices[i].GetSerialNumber() == _config.serial.c_str())
            {
                _camera.Attach(tlFactory.CreateDevice(devices[i]));
                _camera.Open();

                camFound = true;
                break;
            }
        }

        if (!camFound)
        {
            throw std::runtime_error(
                fmt::format("{}: No camera matching serial: {}", _videoStream.id, _config.serial));
        }

        logInfo("{}: Camera resolution: {}x{}",
                _videoStream.id,
                _camera.SensorWidth(),
                _camera.SensorHeight());

        logInfo("{}: Camera region of interest: ({},{}),{}x{}",
                _videoStream.id,
                _camera.OffsetX(),
                _camera.OffsetY(),
                _camera.Width(),
                _camera.Height());

        // TODO: Set cam params as set in config file

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
    }
    catch (Pylon::GenericException e)
    {
        throw std::runtime_error(
            fmt::format("{}: Failed to initialize camera: {}", _videoStream.id, e.what()));
    }
}

void BaslerCamThread::startCapture()
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

void BaslerCamThread::run()
{
    const unsigned int vwidth  = static_cast<unsigned int>(_config.width);
    const unsigned int vheight = static_cast<unsigned int>(_config.height);

    // The camera timestamp will be used to get a more accurate idea of when the image was taken.
    // Software hangups (e.g. short CPU spikes) can thus be mitigated.
    uint64_t                 n_last_camera_tick_count{0};
    uint64_t                 n_last_frame_id{0};
    boost::posix_time::ptime lastCameraTimestamp;

    uint8_t* p_image;
    uint     img_width, img_height;
    uint64_t n_current_frame_id;

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

                // get image data
                img_width          = _grabbed->GetWidth();
                img_height         = _grabbed->GetHeight();
                p_image            = (uint8_t*) _grabbed->GetBuffer();
                n_current_frame_id = _grabbed->GetImageNumber();

                // Get the timestamp
                const auto wallClockNow = boost::posix_time::microsec_clock::universal_time();
                // the camera specific tick count. The Basler acA4096-30um
                // clock freq is 1 GHz (1 tick per 1 ns)
                const uint64_t n_current_camera_tick_count = _grabbed->GetTimeStamp();

                // Image sequence sanity check.
                if (n_last_frame_id != 0 && n_current_frame_id != n_last_frame_id + 1)
                {
                    logWarning(
                        "{}: Camera lost frame: This frame: #{} last frame: #{} timestamp: {}",
                        _videoStream.id,
                        n_current_frame_id,
                        n_last_frame_id,
                        boost::posix_time::to_iso_extended_string(wallClockNow));

                    // add camera handling
                    // ...
                }
                n_last_frame_id = n_current_frame_id;

                // If last timestamp is later than current timestamp, we have
                // to reset the camera timestamps.
                if (n_last_camera_tick_count == 0 ||
                    (n_last_camera_tick_count > n_current_camera_tick_count))
                {
                    if (n_last_camera_tick_count != 0 && lastCameraTimestamp > wallClockNow &&
                        loopCount > 10)
                    {
                        logWarning(
                            "{}: Camera clock faster than wall time. Last camera time: {} wall "
                            "clock: {}",
                            _videoStream.id,
                            boost::posix_time::to_iso_extended_string(lastCameraTimestamp),
                            boost::posix_time::to_iso_extended_string(wallClockNow));
                    }
                    lastCameraTimestamp = wallClockNow;
                }
                else
                {
                    const int64_t tick_delta = static_cast<const int64_t>(
                        n_current_camera_tick_count - n_last_camera_tick_count);
                    // TODO: Add tick to microsecond conversion factor as config setting
                    assert(tick_delta >= 0);
                    lastCameraTimestamp += boost::posix_time::microseconds(tick_delta * 1000);
                }
                n_last_camera_tick_count = n_current_camera_tick_count;

                // Check if processing a frame took longer than X seconds. If
                // so, log the event.
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

        // Move image to buffer for further procession
        // Crop the image to the expected size (e.g. 4000x3000).
        // This is necessary, because the encoder/codec requires the image sizes to be
        // some multiple of X.
        // TODO: Remove this part and add camera check if conforming with size requirements
        cv::Mat            wholeImageMatrix(cv::Size(img_width, img_height),
                                 CV_8UC1, /// FIXME: This should not be hardcoded, the pixel
                                          /// type should be mapped from pylon to opencv
                                 p_image,
                                 cv::Mat::AUTO_STEP);
        const unsigned int marginToBeCroppedX = (img_width > vwidth) ? img_width - vwidth : 0;
        const unsigned int marginToBeCroppedY = (img_height > vheight) ? img_height - vheight : 0;
        if (marginToBeCroppedX > 0 || marginToBeCroppedY > 0)
        {
            const int cropLeft           = marginToBeCroppedX / 2;
            const int cropTop            = marginToBeCroppedY / 2;
            cv::Mat   croppedImageMatrix = wholeImageMatrix(
                cv::Rect(cropLeft, cropTop, static_cast<int>(vwidth), static_cast<int>(vheight)));
            wholeImageMatrix = croppedImageMatrix;
        }
        const std::string frameTimestamp = boost::posix_time::to_iso_extended_string(
                                               lastCameraTimestamp) +
                                           "Z";
        auto buf = std::make_shared<GrayscaleImage>(vwidth, vheight, frameTimestamp);
        memcpy(&buf.get()->data[0], wholeImageMatrix.data, vwidth * vheight);

        _videoStream.push(buf);
    }

    return;
}
