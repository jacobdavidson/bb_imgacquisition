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
    if (!initCamera())
    {
        std::ostringstream msg;
        msg << "Failed to initialize camera " << _config.serial;
        throw std::runtime_error(msg.str());
    }

    if (!startCapture())

    {
        std::ostringstream msg;
        msg << "Failed to start capturing camera " << _config.serial;
        throw std::runtime_error(msg.str());
    }
}

bool BaslerCamThread::initCamera()
{
    // If the configuration entry is invalid, just skip it.
    if (_config.serial.empty())
    {
        sendLogMessage(0, "Serial number not configured. Skipping camera discovery.");
        return false;
    }

    try
    {
        // Get the transport layer factory.
        auto& tlFactory = Pylon::CTlFactory::GetInstance();

        // Get all attached devices and return -1 if no device is found.
        Pylon::DeviceInfoList_t devices;

        if (auto r = tlFactory.EnumerateDevices(devices); r < 0)
        {
            throw std::runtime_error("Coult not enumerate cameras");
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

        if (camFound)
        {
            std::ostringstream msg;
            msg << "Resolution: " << _camera.Width() << "x" << _camera.Height();
            sendLogMessage(0, msg.str());
            // ToDo:
            // print additional info
            // set cam params as set in config file
        }
        else
        {
            sendLogMessage(0, "failed to find cam matching config serial nr: " + _config.serial);
            return false;
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
                        throw std::runtime_error("Invalid camera hardware trigger source");
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
        sendLogMessage(0, e.what());
        return false;
    }

    return true;
}

// This function starts the streaming from the camera
bool BaslerCamThread::startCapture()
{

    try
    {
        _camera.StartGrabbing();
    }
    catch (Pylon::GenericException e)
    {
        return false;
    }

    return true;
}

void BaslerCamThread::run()
{
    SettingsIAC* set = SettingsIAC::getInstance();

    char        logfilepathFull[256];
    std::string logdir = set->logDirectory();
    sprintf(logfilepathFull, logdir.c_str(), _videoStream.id);

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
                        std::cerr << "Error: " << _grabbed->GetErrorCode() << " "
                                  << _grabbed->GetErrorDescription() << std::endl;
                        continue;
                    }
                }
                else
                {
                    throw std::logic_error("Not implemented");
                }

                // get time after getting the data
                const std::chrono::steady_clock::time_point begin =
                    std::chrono::steady_clock::now();

                // get image data
                img_width          = _grabbed->GetWidth();
                img_height         = _grabbed->GetHeight();
                p_image            = (uint8_t*) _grabbed->GetBuffer();
                n_current_frame_id = _grabbed->GetImageNumber();

                // get time after getting the data
                const auto wallClockNow = boost::posix_time::microsec_clock::universal_time();
                const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
                // the camera specific tick count. The Basler acA4096-30um
                // clock freq is 1 GHz (1 tick per 1 ns)
                const uint64_t n_current_camera_tick_count = _grabbed->GetTimeStamp();

                // Image sequence sanity check.
                if (n_last_frame_id != 0 && n_current_frame_id != n_last_frame_id + 1)
                {
                    QString str("Warning: Camera lost frame: This frame: #");
                    str.append(std::to_string(n_current_frame_id).c_str());
                    str.append(" last frame: #");
                    str.append(std::to_string(n_last_frame_id).c_str());
                    str.append(" timestamp: ");
                    str.append(boost::posix_time::to_iso_extended_string(wallClockNow).c_str());
                    std::cerr << str.toStdString() << std::endl;
                    generateLog(logfilepathFull, str);

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
                        QString str(
                            "Warning: camera clock faster than wall "
                            "time. Last camera time: ");
                        str.append(boost::posix_time::to_iso_extended_string(lastCameraTimestamp)
                                       .c_str());
                        str.append(" wall clock: ");
                        str.append(
                            boost::posix_time::to_iso_extended_string(wallClockNow).c_str());
                        std::cerr << str.toStdString() << std::endl;
                        generateLog(logfilepathFull, str);
                    }
                    lastCameraTimestamp = wallClockNow;
                }
                else
                {
                    const int64_t tick_delta = static_cast<const int64_t>(
                        n_current_camera_tick_count - n_last_camera_tick_count);
                    assert(tick_delta >= 0); // ToDo: add tick to microsecond conversion
                                             // factor as config setting
                    lastCameraTimestamp += boost::posix_time::microseconds(tick_delta * 1000);
                }
                n_last_camera_tick_count = n_current_camera_tick_count;

                // Check if processing a frame took longer than X seconds. If
                // so, log the event.
                const int64_t duration =
                    std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
                if (duration > 2 * (1000000 / 6))
                {
                    QString str("Warning: Processing time too long: ");
                    str.append(std::to_string(duration).c_str());
                    str.append(" on camera ");
                    str.append(QString::fromStdString(_videoStream.id));
                    std::cerr << str.toStdString() << std::endl;
                    generateLog(logfilepathFull, str);
                }
            }
            catch (Pylon::GenericException e) // could not retrieve grab result
            {
                std::cerr << e.what() << '\n';

                // ToDo: if the problem persists and we get this error repeatedly,
                // restart application
            }
        }
        else // camera not grabbing
        {
            // although grabbing had been started, isGrabbing returned false. this means
            // somehow the camera stopped grabbing
            // --> we simply log it and restart the application.
            const std::string message = "Aquisition failed with error code " + std::to_string(42);
            logCriticalError(message);
            std::exit(1);
        }

        // Move image to buffer for further procession
        // Crop the image to the expected size (e.g. 4000x3000).
        // This is necessary, because the encoder/codec requires the image sizes to be
        // some multiple of X. ToDo: remove this part and add camera check if
        // conforming with size requirements
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

void BaslerCamThread::logCriticalError(const std::string& message)
{
    sendLogMessage(0, message);
}

void BaslerCamThread::sendLogMessage(int logLevel, QString message)
{
    emit logMessage(logLevel, "Cam " + QString::fromStdString(_videoStream.id) + " : " + message);
}

void BaslerCamThread::sendLogMessage(int logLevel, std::string message)
{
    sendLogMessage(logLevel, QString::fromStdString(message));
}

void BaslerCamThread::sendLogMessage(int logLevel, const char* message)
{
    sendLogMessage(logLevel, QString(message));
}

void BaslerCamThread::generateLog(QString path, QString message)
{
    boost::filesystem::create_directories({path.toStdString()});
    QString filename = (path + "log.txt");
    QFile   file(filename);
    file.open(QIODevice::Append);
    QTextStream stream(&file);
    stream << QString::fromStdString(getTimestamp()) << ": " << message << "\r\n";
    file.close();
}
