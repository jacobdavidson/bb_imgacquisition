// SPDX-License-Identifier: GPL-3.0-or-later

#include "XimeaCamThread.h"

#include <cstdlib>
#include <iostream>

#include <QDebug>
#include <qdir.h> //QT stuff
#include <qtextstream.h>

#include "Watchdog.h"
#include "settings/Settings.h"
#include "settings/utility.h"
#include "GrayscaleImage.h"

#include <sstream>

#include <array>
#include <ctime>
#include <time.h>

#include <opencv2/opencv.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time.hpp>

XimeaCamThread::XimeaCamThread(Config config, VideoStream videoStream, Watchdog* watchdog)
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

bool XimeaCamThread::initCamera()
{
    // Try settings that our mc124mg does not support.
    // This might become relevant with other XIMEA cams.
    // However, with our camera, it just displays warnings.
    const bool tryAllXimeaCamSettings{false};
    DWORD      numberOfDevices{0};
    XI_RETURN  errorCode{XI_OK};

    // Disable bandwith-measuring for Ximea cams.
    // According to docs, this can be wonky with multiple cameras.
    xiSetParamInt(0, XI_PRM_AUTO_BANDWIDTH_CALCULATION, XI_OFF);
    if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_AUTO_BANDWIDTH_CALCULATION"))
        return false;

    errorCode = xiGetNumberDevices(&numberOfDevices);
    if (!checkReturnCode(errorCode, "xiGetNumberDevices"))
        return false;

    SettingsIAC* set = SettingsIAC::getInstance();

    // If the configuration entry is invalid, just skip it.
    if (_config.serial.empty())
    {
        sendLogMessage(0, "Serial number not configured. Skipping camera discovery.");
        return false;
    }

    DWORD numCameras;
    if (auto err = xiGetNumberDevices(&numCameras); err != XI_OK)
    {
        throw std::runtime_error("Could not enumerate cameras");
    }

    bool camFound = false;
    for (decltype(numCameras) i = 0; i < numCameras; ++i)
    {
        XI_RETURN errorCode;
        HANDLE    cam;
        errorCode = xiOpenDevice(i, &cam);
        if (errorCode != XI_OK)
        {
            // Maybe the cam has already been opened by another thread.
            continue;
        }
        char serial[20] = "";
        errorCode       = xiGetParamString(cam, XI_PRM_DEVICE_SN, serial, sizeof(serial));
        if (errorCode != XI_OK)
        {
            qDebug() << "Error checking serial of cam #" << i;
        }
        else
        {
            if (_config.serial == serial)
            {
                camFound = true;
                _Camera  = cam;
                // Do NOT close the camera.
                break;
            }
        }
        // Close the camera if mismatch.
        xiCloseDevice(cam);
    }

    if (!camFound)
    {
        sendLogMessage(0, "failed to find cam matching config serial nr: " + _config.serial);
        return false;
    }
    else
    {
        sendLogMessage(0, "OPEN! serial nr: " + _config.serial);
        // Print some interesting meta data.

        // Temperature.
        if (tryAllXimeaCamSettings)
        {
            float temperature = 0;
            errorCode = xiSetParamInt(_Camera, XI_PRM_TEMP_SELECTOR, XI_TEMP_INTERFACE_BOARD);
            if (errorCode == XI_OK)
            {
                errorCode = xiGetParamFloat(_Camera, XI_PRM_TEMP, &temperature);
                if (errorCode == XI_OK)
                {
                    std::ostringstream out;
                    out << "Sensor temperature at " << std::setprecision(5) << std::fixed
                        << temperature << "°C";
                    sendLogMessage(0, out.str());
                }
                else
                {
                    sendLogMessage(0, "Reading temperature failed.");
                }
            }
            else
            {
                sendLogMessage(0, "Temperature unavailable.");
            }
        }
        if (tryAllXimeaCamSettings)
        {
            float focusDistance = 0;
            errorCode = xiGetParamFloat(_Camera, XI_PRM_LENS_FOCUS_DISTANCE, &focusDistance);
            if (errorCode == XI_OK)
            {
                std::ostringstream out;
                out << "Focus distance at " << std::setprecision(5) << std::fixed << focusDistance
                    << "cm";
                sendLogMessage(0, out.str());
            }
            else
            {
                sendLogMessage(0, "Focus distance unavailable.");
            }
        }
    }

    if (_config.throughput_limit)
    {
        // Set the bandwidth limit.
        errorCode = xiSetParamInt(_Camera, XI_PRM_LIMIT_BANDWIDTH, *_config.throughput_limit);
        if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_LIMIT_BANDWIDTH"))
            return false;
        errorCode = xiSetParamInt(_Camera, XI_PRM_LIMIT_BANDWIDTH_MODE, XI_ON);
        if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_LIMIT_BANDWIDTH_MODE"))
            return false;
    }

    // Set a format that corresponds to the Y in YUV.
    errorCode = xiSetParamInt(_Camera, XI_PRM_IMAGE_DATA_FORMAT, XI_RAW8);
    if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_IMAGE_DATA_FORMAT"))
        return false;

    // Deactivate downsampling.
    if (tryAllXimeaCamSettings)
    {
        errorCode = xiSetParamInt(_Camera, XI_PRM_DOWNSAMPLING, 1);
        if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_IMAGE_DATA_FORMAT"))
            return false;
    }

    // Select configured properties.

    // Exposure
    if (!_config.exposure || !*_config.exposure)
    {
        throw std::runtime_error("Manual camera exposure required");
    }

    // Disable auto-exposure.
    errorCode = xiSetParamInt(_Camera, XI_PRM_AEAG, 0);
    if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_AEAG"))
        return false;

    errorCode = xiSetParamInt(_Camera, XI_PRM_EXPOSURE, **_config.exposure);
    if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_EXPOSURE"))
        return false;

    // Disable burst exposure - the default is off, but we make sure here.
    xiSetParamInt(_Camera, XI_PRM_TRG_SELECTOR, XI_TRG_SEL_FRAME_START);
    xiSetParamInt(_Camera, XI_PRM_EXPOSURE_BURST_COUNT, 1);

    // The Ximea camera does not support 4000x3000 pixels.
    // errorCode = xiSetParamInt(_Camera, XI_PRM_WIDTH, 4000);
    // if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_WIDTH")) return false;
    // errorCode = xiSetParamInt(_Camera, XI_PRM_HEIGHT, 3000);
    // if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_HEIGHT")) return false;

    // Gain.
    if (!_config.gain || !*_config.gain)
    {
        throw std::runtime_error("Manual camera gain required");
    }
    errorCode = xiSetParamInt(_Camera, XI_PRM_GAIN_SELECTOR, XI_GAIN_SELECTOR_ALL);
    if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_GAIN_SELECTOR"))
        return false;
    errorCode = xiSetParamFloat(_Camera, XI_PRM_GAIN, static_cast<float>(**_config.gain));
    if (!checkReturnCode(errorCode, "xiSetParamFloat XI_PRM_GAIN"))
        return false;

    // Whitebalance.
    if (tryAllXimeaCamSettings && _config.whitebalance)
    {
        errorCode = xiSetParamInt(_Camera, XI_PRM_AUTO_WB, *_config.whitebalance);
        checkReturnCode(errorCode, "xiSetParamInt XI_PRM_AUTO_WB");
    }

    if (std::holds_alternative<Config::SoftwareTrigger>(_config.trigger))
    {
        errorCode = xiSetParamInt(_Camera, XI_PRM_ACQ_TIMING_MODE, XI_ACQ_TIMING_MODE_FRAME_RATE);
        if (errorCode == XI_OK)
            xiSetParamFloat(
                _Camera,
                XI_PRM_FRAMERATE,
                static_cast<float>(
                    std::get<Config::SoftwareTrigger>(_config.trigger).framesPerSecond));

        errorCode = xiSetParamInt(_Camera, XI_PRM_TRG_SOURCE, XI_TRG_OFF);
        if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_TRG_SOURCE"))
            return false;
    }
    else if (std::holds_alternative<Config::HardwareTrigger>(_config.trigger))
    {
        errorCode = xiSetParamInt(_Camera,
                                  XI_PRM_GPI_SELECTOR,
                                  std::get<Config::HardwareTrigger>(_config.trigger).source);
        if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_GPI_SELECTOR"))
            return false;
        errorCode = xiSetParamInt(_Camera, XI_PRM_GPI_MODE, XI_GPI_TRIGGER);
        if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_GPI_MODE"))
            return false;

        errorCode = xiSetParamInt(_Camera, XI_PRM_TRG_SOURCE, XI_TRG_EDGE_RISING);
        if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_TRG_SOURCE"))
            return false;
    }

    // The default is no buffering.
    if (_config.buffer_size)
    {
        const auto buffer_size = *_config.buffer_size;

        errorCode = xiSetParamInt(_Camera, XI_PRM_BUFFERS_QUEUE_SIZE, buffer_size + 1);
        if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_BUFFERS_QUEUE_SIZE"))
            return false;
        errorCode = xiSetParamInt(_Camera, XI_PRM_RECENT_FRAME, buffer_size == 0 ? 1 : 0);
        if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_RECENT_FRAME"))
            return false;
        if (buffer_size > 0)
        {
            errorCode = xiSetParamInt(_Camera,
                                      XI_PRM_ACQ_BUFFER_SIZE,
                                      3 * (buffer_size + 1) * 3008 * 4112);
            if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_ACQ_BUFFER_SIZE"))
                return false;
        }
    }

    // errorCode = xiSetParamInt(_Camera, XI_PRM_BUFFER_POLICY, XI_BP_SAFE);
    // if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_BUFFER_POLICY")) return false;

    // Set maximum transport buffer size.
    {
        int max_transport_buf_size{0};
        errorCode = xiGetParamInt(_Camera,
                                  XI_PRM_ACQ_TRANSPORT_BUFFER_SIZE XI_PRM_INFO_MAX,
                                  &max_transport_buf_size);
        if (!checkReturnCode(errorCode,
                             "xiGetParamInt XI_PRM_ACQ_TRANSPORT_BUFFER_SIZE XI_PRM_INFO_MAX"))
            return false;
        errorCode = xiSetParamInt(_Camera,
                                  XI_PRM_ACQ_TRANSPORT_BUFFER_SIZE,
                                  max_transport_buf_size);
        if (!checkReturnCode(errorCode, "xiSetParamInt XI_PRM_ACQ_TRANSPORT_BUFFER_SIZE"))
            return false;
    }

    return true;
}

// This function starts the streaming from the camera
bool XimeaCamThread::startCapture()
{
    auto errorCode = xiStartAcquisition(_Camera);
    if (errorCode != XI_OK)
    {
        sendLogMessage(0, "xiStartAcquisition failed with code " + std::to_string(errorCode));
        return false;
    }
    return true;
}

void XimeaCamThread::run()
{
    SettingsIAC* set = SettingsIAC::getInstance();

    char        logfilepathFull[256];
    std::string logdir = set->logDirectory();
    sprintf(logfilepathFull, logdir.c_str(), _videoStream.id);

    const unsigned int vwidth  = static_cast<unsigned int>(_config.width);
    const unsigned int vheight = static_cast<unsigned int>(_config.height);

    // The camera timestamp will be used to get a more accurate idea of when the image was taken.
    // Software hangups (e.g. short CPU spikes) can thus be mitigated.
    uint64_t                 lastCameraTimestampMicroseconds{0};
    uint64_t                 lastImageSequenceNumber{0};
    boost::posix_time::ptime lastCameraTimestamp;

    // Preallocate image buffer on stack in order to save performance later.
    std::array<unsigned char, 3008 * 4112> imageBuffer;

    for (size_t loopCount = 0; !isInterruptionRequested(); loopCount += 1)
    {
        _watchdog->pulse();

        XI_IMG image;
        image.size    = sizeof(XI_IMG);
        image.bp      = static_cast<LPVOID>(&imageBuffer[0]);
        image.bp_size = imageBuffer.size();

        const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        // Retrieve image and metadata
        auto returnCode = xiGetImage(_Camera, 1000 * 2, &image);
        // Get the timestamp
        const auto wallClockNow = boost::posix_time::microsec_clock::universal_time();
        const std::chrono::steady_clock::time_point end   = std::chrono::steady_clock::now();
        const uint64_t currentCameraTimestampMicroseconds = image.tsUSec;

        // Image sequence sanity check.
        if (lastImageSequenceNumber != 0 && image.nframe != lastImageSequenceNumber + 1)
        {
            QString str("Warning: Camera lost frame: This frame: #");
            str.append(std::to_string(image.nframe).c_str());
            str.append(" last frame: #");
            str.append(std::to_string(lastImageSequenceNumber).c_str());
            str.append(" timestamp: ");
            str.append(boost::posix_time::to_iso_extended_string(wallClockNow).c_str());
            std::cerr << str.toStdString() << std::endl;
            generateLog(logfilepathFull, str);

            for (auto& what : std::map<std::string, int>{
                     {"Transport layer loss: ", XI_CNT_SEL_TRANSPORT_SKIPPED_FRAMES},
                     {"API layer loss: ", XI_CNT_SEL_API_SKIPPED_FRAMES}})
            {
                xiSetParamInt(_Camera, XI_PRM_COUNTER_SELECTOR, what.second);
                int result{0};
                xiGetParamInt(_Camera, XI_PRM_COUNTER_VALUE, &result);
                std::cerr << what.first << result << std::endl;
            }
        }
        lastImageSequenceNumber = image.nframe;

        // Reset the camera timestamp?
        if (lastCameraTimestampMicroseconds == 0 ||
            (lastCameraTimestampMicroseconds > currentCameraTimestampMicroseconds))
        {
            if (lastCameraTimestampMicroseconds != 0 && lastCameraTimestamp > wallClockNow &&
                loopCount > 10)
            {
                QString str("Warning: camera clock faster than wall time. Last camera time: ");
                str.append(boost::posix_time::to_iso_extended_string(lastCameraTimestamp).c_str());
                str.append(" wall clock: ");
                str.append(boost::posix_time::to_iso_extended_string(wallClockNow).c_str());
                std::cerr << str.toStdString() << std::endl;
                generateLog(logfilepathFull, str);
            }
            lastCameraTimestamp = wallClockNow;
        }
        else
        {
            const int64_t microsecondDelta = static_cast<const int64_t>(
                currentCameraTimestampMicroseconds - lastCameraTimestampMicroseconds);
            assert(microsecondDelta >= 0);
            lastCameraTimestamp += boost::posix_time::microseconds(microsecondDelta);
        }
        lastCameraTimestampMicroseconds = currentCameraTimestampMicroseconds;

        // Skip the first X images to allow the camera buffer to be flushed.
        if (loopCount < 10)
        {
            continue;
        }

        // Check if processing a frame took longer than X seconds. If so, log the event.
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

        // In case an error occurs, simply log it and restart the application.
        // Sometimes fetching images starts to fail and cameras need to be re-initialized.
        // However, this will solve the issue in the general case
        if (returnCode != XI_OK)
        {
            const std::string message = "Aquisition failed with error code " +
                                        std::to_string(returnCode);
            logCriticalError(message, message);
            std::exit(1);
        }

        // Crop the image to the expected size (e.g. 4000x3000).
        // This is necessary, because the encoder/codec requires the image sizes to be
        // some multiple of X.
        cv::Mat wholeImageMatrix(
            cv::Size(static_cast<int>(image.width), static_cast<int>(image.height)),
            CV_8UC1,
            image.bp,
            cv::Mat::AUTO_STEP);
        const unsigned int marginToBeCroppedX = (image.width > vwidth) ? image.width - vwidth : 0;
        const unsigned int marginToBeCroppedY = (image.height > vheight) ? image.height - vheight
                                                                         : 0;
        if (marginToBeCroppedX > 0 || marginToBeCroppedY > 0)
        {
            const int cropLeft           = marginToBeCroppedX / 2;
            const int cropTop            = marginToBeCroppedY / 2;
            cv::Mat   croppedImageMatrix = wholeImageMatrix(
                cv::Rect(cropLeft, cropTop, static_cast<int>(vwidth), static_cast<int>(vheight)));
            croppedImageMatrix.copyTo(wholeImageMatrix);
        }
        const std::string frameTimestamp = boost::posix_time::to_iso_extended_string(
                                               lastCameraTimestamp) +
                                           "Z";
        auto buf = std::make_shared<GrayscaleImage>(vwidth, vheight, frameTimestamp);
        memcpy(&buf.get()->data[0], wholeImageMatrix.data, vwidth * vheight);

        _videoStream.push(buf);
    }
    // This code will never be executed.
    assert(false);
    auto errorCode = xiStopAcquisition(_Camera);
    checkReturnCode(errorCode, "xiStopAcquisition");
    errorCode = xiCloseDevice(_Camera);
    checkReturnCode(errorCode, "xiCloseDevice");

    std::cerr << "done" << std::endl;
    return;
}
/*
void XimeaCamThread::logCriticalError(cv::Error e) {
    std::ostringstream str;
    str << "Error acquiring image. Printing full info and exiting. "
        << std::endl;
    str << "Type: "                 << e.GetType()              << std::endl;
    str << "Description: "          << e.GetDescription()       << std::endl;
    str << "Filename: "             << e.GetFilename()          << std::endl;
    const std::string shortMsg = "Short log: \n" + str.str();

    str << "Line: "                 << e.GetLine()              << std::endl;
    str << "ErrorType: "            << e.GetType()              << std::endl;
    cv::Error cause = e.GetCause();
    str << "Cause.Type: "           << cause.GetType()          << std::endl;
    str << "Cause.Description: "    << cause.GetDescription()   << std::endl;
    str << "Cause.Filename: "       << cause.GetFilename()      << std::endl;
    str << "Cause.Line: "           << cause.GetLine()          << std::endl;
    str << "Cause.ErrorType: "      << cause.GetType()          << std::endl;
    str << "Exit! "                                             << std::endl;
    logCriticalError(shortMsg, str.str());
}
*/

void XimeaCamThread::logCriticalError(const std::string& shortMsg, const std::string& message)
{
    if (!message.empty())
    {
        char              logfilepathFull[256];
        std::stringstream str;
        SettingsIAC*      set    = SettingsIAC::getInstance();
        std::string       logdir = set->logDirectory();
        sprintf(logfilepathFull, logdir.c_str(), _videoStream.id);
        generateLog(logfilepathFull, message.c_str());
    }
}

// Just prints the camera's info
/*
void XimeaCamThread::PrintCameraInfo(CameraInfo *pCamInfo) {
    sendLogMessage(3,
                   QString() + "\n*** CAMERA INFORMATION ***\n" + "Serial number - "
                   + QString::number(pCamInfo->serialNumber) + "\n"
                   + "Camera model - " + QString(pCamInfo->modelName) + "\n"
                   + "Camera vendor - " + QString(pCamInfo->vendorName) + "\n"
                   + "Sensor - " + QString(pCamInfo->sensorInfo) + "\n"
                   + "Resolution - " + QString(pCamInfo->sensorResolution)
                   + "\n" + "Firmware version - "
                   + QString(pCamInfo->firmwareVersion) + "\n"
                   + "Firmware build time - "
                   + QString(pCamInfo->firmwareBuildTime) + "\n" + "\n");
}*/

bool XimeaCamThread::checkReturnCode(XI_RETURN errorCode, const std::string& operation)
{
    if (errorCode != XI_OK)
    {
        sendLogMessage(1,
                       "[" + QString::fromStdString(operation) + "] error code " +
                           QString::number(errorCode));
        return false;
    }
    return true;
}

void XimeaCamThread::sendLogMessage(int logLevel, QString message)
{
    emit logMessage(logLevel, "Cam " + QString::fromStdString(_videoStream.id) + " : " + message);
}
void XimeaCamThread::sendLogMessage(int logLevel, std::string message)
{
    sendLogMessage(logLevel, QString::fromStdString(message));
}
void XimeaCamThread::sendLogMessage(int logLevel, const char* message)
{
    sendLogMessage(logLevel, QString(message));
}

void XimeaCamThread::generateLog(QString path, QString message)
{
    boost::filesystem::create_directories({path.toStdString()});
    QString filename = (path + "log.txt");
    QFile   file(filename);
    file.open(QIODevice::Append);
    QTextStream stream(&file);
    stream << QString::fromStdString(getTimestamp()) << ": " << message << "\r\n";
    file.close();
}
