// SPDX-License-Identifier: GPL-3.0-or-later

#include "Flea3CamThread.h"

#include <cstdlib>
#include <iostream>

#include <qdir.h> //QT stuff
#include <qtextstream.h>

#include <FlyCapture2.h>

#include "format.h"

#include "Watchdog.h"
#include "settings/Settings.h"
#include "settings/utility.h"
#include "GrayscaleImage.h"

#include <sstream> //stringstreams

#include <ctime> //get time
#include <time.h>

#if __linux__
    #include <unistd.h> //sleep
    #include <sys/time.h>
#else
    #include <stdint.h>
#endif

#include <opencv2/opencv.hpp>

// Flea3CamThread constructor
Flea3CamThread::Flea3CamThread(Config config, VideoStream videoStream, Watchdog* watchdog)
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

bool Flea3CamThread::initCamera()
{
    // SET VIDEO MODE HERE!!!
    FlyCapture2::Format7Info fmt7Info;

    const FlyCapture2::Mode        fmt7Mode   = FlyCapture2::MODE_10;
    const FlyCapture2::PixelFormat fmt7PixFmt = FlyCapture2::PIXEL_FORMAT_RAW8;

    FlyCapture2::Format7ImageSettings fmt7ImageSettings;

    fmt7ImageSettings.mode        = fmt7Mode;
    fmt7ImageSettings.offsetX     = 0;
    fmt7ImageSettings.offsetY     = 0;
    fmt7ImageSettings.width       = _config.width;
    fmt7ImageSettings.height      = _config.height;
    fmt7ImageSettings.pixelFormat = fmt7PixFmt;

    FlyCapture2::Format7PacketInfo fmt7PacketInfo;
    // fmt7PacketInfo.recommendedBytesPerPacket = 5040;

    FlyCapture2::BusManager busMgr;
    FlyCapture2::PGRGuid    guid;
    FlyCapture2::CameraInfo camInfo;

    FlyCapture2::FC2Config BufferFrame;

    FlyCapture2::EmbeddedImageInfo EmbeddedInfo;

    // Properties to be modified

    FlyCapture2::Property brightness;
    brightness.type = FlyCapture2::BRIGHTNESS;

    FlyCapture2::Property exposure;
    exposure.type = FlyCapture2::AUTO_EXPOSURE;

    FlyCapture2::Property shutter;
    shutter.type = FlyCapture2::SHUTTER;

    FlyCapture2::Property frmRate;
    frmRate.type = FlyCapture2::FRAME_RATE;

    FlyCapture2::Property gain;
    gain.type = FlyCapture2::GAIN;

    FlyCapture2::Property wBalance;
    wBalance.type = FlyCapture2::WHITE_BALANCE;

    bool supported;

    unsigned int numCameras;
    if (auto err = busMgr.GetNumOfCameras(&numCameras); err != FlyCapture2::PGRERROR_OK)
    {
        std::ostringstream msg;
        msg << "Could not enumerate cameras: " << err.GetDescription();
        throw std::runtime_error(msg.str());
    }

    bool         camFound = false;
    unsigned int camIndex;
    unsigned int serial;
    for (decltype(numCameras) i = 0; i < numCameras; i++)
    {
        if (auto err = busMgr.GetCameraSerialNumberFromIndex(i, &serial);
            err != FlyCapture2::PGRERROR_OK)
        {
            std::ostringstream msg;
            msg << "Could not read camera serial number: " << err.GetDescription();
            throw std::runtime_error(msg.str());
        }
        if (serial == std::stoull(_config.serial))
        {
            camFound = true;
            camIndex = i;
            break;
        }
    }

    if (!camFound)
    {
        return false;
    }

    sendLogMessage(3,
                   "Camera " + QString::fromStdString(_videoStream.id) + "(ID) Serial Number: " +
                       QString::number(serial)); // serial number is printed

    // Gets the PGRGuid from the camera
    if (!checkReturnCode(busMgr.GetCameraFromIndex(camIndex, &guid)))
    {
        return false;
    }

    // Connect to camera
    if (!checkReturnCode(_Camera.Connect(&guid)))
    {
        return false;
    }

    // Get the camera information
    if (!checkReturnCode(_Camera.GetCameraInfo(&camInfo)))
    {
        return false;
    }

    PrintCameraInfo(&camInfo); // camera information is printed

    /////////////////// ALL THE PROCESS WITH FORMAT 7 ////////////////////////////////

    // Query for available Format 7 modes
    if (!checkReturnCode(_Camera.GetFormat7Info(&fmt7Info, &supported)))
    {
        return false;
    }

    // Print the camera capabilities for fmt7
    PrintFormat7Capabilities(fmt7Info);

    // Validate the settings to make sure that they are valid
    if (!checkReturnCode(
            _Camera.ValidateFormat7Settings(&fmt7ImageSettings, &supported, &fmt7PacketInfo)))
    {
        return false;
    }

    if (!supported)
    {
        // Settings are not valid
        sendLogMessage(1, "Format7 settings are not valid");
        return false;
    }

    // Set the settings to the camera
    if (!checkReturnCode(
            _Camera.SetFormat7Configuration(&fmt7ImageSettings,
                                            fmt7PacketInfo.recommendedBytesPerPacket)))
    {
        return false;
    }

    /////////////////////////////// ENDS PROCESS WITH FORMAT 7
    /////////////////////////////////////////////

    /////////////////// ALL THE PROCESS WITH FC2Config  ////////////////////////////////

    if (_config.buffer_size)
    {
        // Grab the current configuration from the camera in ss_BufferFrame
        if (!checkReturnCode(_Camera.GetConfiguration(&BufferFrame)))
        {
            return false;
        }

        // Modify the maximum number of frames to be buffered and send it back to the camera
        BufferFrame.numBuffers = *_config.buffer_size;
        BufferFrame.grabMode   = FlyCapture2::BUFFER_FRAMES;

        if (!checkReturnCode(_Camera.SetConfiguration(&BufferFrame)))
        {
            return false;
        }
    }

    /////////////////// ENDS PROCESS WITH FC2Config  ////////////////////////////////

    /////////////////// ALL THE PROCESS WITH EMBEDDEDIMAGEINFO  ////////////////////////////////

    // Get the info of the image in ss_EmbeddedInfo
    if (!checkReturnCode(_Camera.GetEmbeddedImageInfo(&EmbeddedInfo)))
    {
        return false;
    }

    // Again modify a couple of parameters and send them back to the camera
    if (EmbeddedInfo.timestamp.available == true)
    {
        // if possible activates timestamp
        EmbeddedInfo.timestamp.onOff = true;
    }
    else
    {
        sendLogMessage(3, "Timestamp is not available!");
    }

    if (EmbeddedInfo.frameCounter.available == true)
    {
        // if possible activates frame counter
        EmbeddedInfo.frameCounter.onOff = true;
    }
    else
    {
        sendLogMessage(3, "Framecounter is not avalable!");
    }

    if (!checkReturnCode(_Camera.SetEmbeddedImageInfo(&EmbeddedInfo)))
    {
        return false;
    }

    /////////////////// ENDS PROCESS WITH EMBEDDEDIMAGEINFO  ////////////////////////////////

    /////////////////// ALL THE PROCESS WITH PROPERTIES  ////////////////////////////////

    //-------------------- BRIGHTNESS STARTS        -----------------------------------
    if (!checkReturnCode(_Camera.GetProperty(&brightness)))
    {
        return false;
    }

    brightness.onOff = static_cast<bool>(_config.blacklevel);
    if (brightness.onOff)
    {
        brightness.autoManualMode = !static_cast<bool>(*_config.blacklevel);
        if (!brightness.autoManualMode)
        {
            brightness.absValue = **_config.blacklevel;
        }
    }

    if (!checkReturnCode(_Camera.SetProperty(&brightness)))
    {
        return false;
    }

    if (!checkReturnCode(_Camera.GetProperty(&brightness)))
    {
        return false;
    }

    sendLogMessage(3,
                   QString::fromStdString(fmt::format("Brightness Parameter is: {} and {:.2f}",

                                                      brightness.onOff ? "On" : "Off",
                                                      brightness.absValue)));

    //-------------------- BROGHTNESS ENDS          -----------------------------------

    //-------------------- EXPOSURE STARTS          -----------------------------------
    if (!checkReturnCode(_Camera.GetProperty(&exposure)))
    {
        return false;
    }

    exposure.onOff = static_cast<bool>(_config.exposure);
    if (exposure.onOff)
    {
        exposure.autoManualMode = !static_cast<bool>(*_config.exposure);
        if (!exposure.autoManualMode)
        {
            exposure.absValue = **_config.exposure;
        }
    }

    if (!checkReturnCode(_Camera.SetProperty(&exposure)))
    {
        return false;
    }

    if (!checkReturnCode(_Camera.GetProperty(&exposure)))
    {
        return false;
    }

    sendLogMessage(3,
                   "Exposure Parameter is: " +
                       QString().sprintf("%s and %s",
                                         exposure.onOff ? "On" : "Off",
                                         exposure.autoManualMode ? "Auto" : "Manual"));

    //-------------------- EXPOSURE ENDS -----------------------------------

    //-------------------- SHUTTER STARTS           -----------------------------------
    if (!checkReturnCode(_Camera.GetProperty(&shutter)))
    {
        return false;
    }

    shutter.onOff = static_cast<bool>(_config.shutter);
    if (shutter.onOff)
    {
        shutter.autoManualMode = !static_cast<bool>(*_config.shutter);
        if (!shutter.autoManualMode)
        {
            shutter.absValue = **_config.shutter;
        }
    }

    if (!checkReturnCode(_Camera.SetProperty(&shutter)))
    {
        return false;
    }

    if (!checkReturnCode(_Camera.GetProperty(&shutter)))
    {
        return false;
    }

    sendLogMessage(3,
                   "New shutter parameter is: " +
                       QString().sprintf("%s and %.2f ms",
                                         shutter.autoManualMode ? "Auto" : "Manual",
                                         shutter.absValue));
    //-------------------- SHUTTER ENDS             -----------------------------------

    //-------------------- GAIN STARTS              -----------------------------------
    if (!checkReturnCode(_Camera.GetProperty(&gain)))
    {
        return false;
    }

    gain.onOff = static_cast<bool>(_config.gain);
    if (gain.onOff)
    {
        gain.autoManualMode = !static_cast<bool>(*_config.gain);
        if (!gain.autoManualMode)
        {
            gain.absValue = **_config.gain;
        }
    }

    if (!checkReturnCode(_Camera.SetProperty(&gain)))
    {
        return false;
    }

    if (!checkReturnCode(_Camera.GetProperty(&gain)))
    {
        return false;
    }

    sendLogMessage(3,
                   "New gain parameter is: " +
                       QString().sprintf("%s", gain.autoManualMode ? "Auto" : "Manual"));
    //-------------------- GAIN ENDS                -----------------------------------

    //-------------------- WHITE BALANCE STARTS     -----------------------------------
    if (!checkReturnCode(_Camera.GetProperty(&wBalance)))
    {
        return false;
    }

    wBalance.onOff = static_cast<bool>(_config.whitebalance) && *_config.whitebalance;

    if (!checkReturnCode(_Camera.SetProperty(&wBalance)))
    {
        return false;
    }

    if (!checkReturnCode(_Camera.GetProperty(&wBalance)))
    {
        return false;
    }

    sendLogMessage(3,
                   "New White Balance parameter is: " +
                       QString().sprintf("%s", wBalance.onOff ? "On" : "Off"));
    //-------------------- WHITE BALANCE ENDS       -----------------------------------

    //-------------------- TRIGGER MODE STARTS      -----------------------------------

    if (std::holds_alternative<Config::HardwareTrigger>(_config.trigger))
    {
        // Get current trigger settings
        FlyCapture2::TriggerMode triggerMode;
        if (!checkReturnCode(_Camera.GetTriggerMode(&triggerMode)))
        {
            return false;
        }

        // Set camera to trigger mode 0
        triggerMode.onOff     = true;
        triggerMode.mode      = 0;
        triggerMode.parameter = 0;

        // Triggering the camera externally using source 0.
        triggerMode.source = std::get<Config::HardwareTrigger>(_config.trigger).source;

        if (!checkReturnCode(_Camera.SetTriggerMode(&triggerMode)))
        {
            return false;
        }
    }

    //-------------------- TRIGGER MODE ENDS        -----------------------------------

    //-------------------- FRAME RATE STARTS        -----------------------------------

    if (std::holds_alternative<Config::SoftwareTrigger>(_config.trigger))
    {
        if (!checkReturnCode(_Camera.GetProperty(&frmRate)))
        {
            return false;
        }

        frmRate.absControl     = true;
        frmRate.onOff          = true;
        frmRate.autoManualMode = false;
        frmRate.absValue = std::get<Config::SoftwareTrigger>(_config.trigger).framesPerSecond;

        if (!checkReturnCode(_Camera.SetProperty(&frmRate)))
        {
            return false;
        }

        if (!checkReturnCode(_Camera.GetProperty(&frmRate)))
        {
            return false;
        }

        sendLogMessage(3,
                       "New frame rate is " + QString().sprintf("%.2f", frmRate.absValue) +
                           " fps");
    }

    //-------------------- FRAME RATE ENDS          -----------------------------------

    /////////////////// ALL THE PROCESS WITH EMBEDDEDIMAGEINFO  ////////////////////////////////
    // Saves the configuration to memory channel 2
    if (!checkReturnCode(_Camera.SaveToMemoryChannel(2)))
    {
        return false;
    }

    return true;
}

// This function starts the streaming from the camera
bool Flea3CamThread::startCapture()
{
    // Start isochronous image capture
    return checkReturnCode(_Camera.StartCapture());
}

// this is what the function does with the information set in configure
void Flea3CamThread::run()
{
    SettingsIAC* set = SettingsIAC::getInstance();

    char        logfilepathFull[256];
    std::string logdir = set->logDirectory();
    sprintf(logfilepathFull, logdir.c_str(), _videoStream.id);

    int                     vwidth  = _config.width;
    int                     vheight = _config.height;
    FlyCapture2::BusManager busMgr;
    FlyCapture2::PGRGuid    guid;

    unsigned int oldTimeUs = 1000000;
    unsigned int difTimeStampUs;

    auto begin = std::chrono::steady_clock::now();
    auto end   = std::chrono::steady_clock::now();

    ////////////////////////LINUX/////////////////////
#ifdef __linux__
    // Timestamp housekeeping
    time_t          rawtime;
    struct timeval  tv;
    struct timezone tz;
    int             cs;
    int64_t         startTime;

    gettimeofday(&tv, &tz);
    startTime = tv.tv_sec; // Note: using this method the start might be shifted by <1sec
    // The first frame usually contains weird metadata. So we grab it and discard it.
    {
        FlyCapture2::Image cimg;
        _Camera.RetrieveBuffer(&cimg);
        _Camera.RetrieveBuffer(&cimg);
        _TimeStamp = cimg.GetTimeStamp();
        cs         = _TimeStamp.cycleSeconds;
    }
#endif
    ////////////////////////////////////////////////////

    while (!isInterruptionRequested())
    {
        _watchdog->pulse();
        FlyCapture2::Image cimg;

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        // Retrieve image and metadata
        FlyCapture2::Error e = _Camera.RetrieveBuffer(&cimg);
        // Get the timestamp
        std::string currentTimestamp = get_utc_time();

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        // Check if processing a frame took longer than 0.4 seconds. If so, log the event.
        if (const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(begin -
                                                                                        end);
            duration > std::chrono::milliseconds(400))
        {
            const auto msg = fmt::format(
                "Warning: Video Stream {}: Processing time too long: {}\n",
                _videoStream.id,
                duration);
            sendLogMessage(1, QString::fromStdString(msg));
        }

        // In case an error occurs, simply log it and restart the application.
        // Sometimes fetching images starts to fail and cameras need to be re-initialized.
        // However, this will solve the issue in the general case
        if (e.GetType() != FlyCapture2::PGRERROR_OK)
        {
            logCriticalError(e);
            throw std::runtime_error(e.GetDescription());
        }

        // Grab metadata and timestamps from the images
        _ImInfo      = cimg.GetMetadata();
        _FrameNumber = _ImInfo.embeddedFrameCounter;
        _TimeStamp   = cimg.GetTimeStamp();

        //////////////////////////////////////////////

        if (_TimeStamp.microSeconds > oldTimeUs)
        {
            difTimeStampUs = _TimeStamp.microSeconds - oldTimeUs;
        }
        else
        {
            difTimeStampUs = _TimeStamp.microSeconds - oldTimeUs + 1000000;
        }

        oldTimeUs = _TimeStamp.microSeconds;

        ///////////////////////////////////

        // Prepare and put the image into the buffer
        // std::string currentTimestamp(timeresult);

        // Move image to buffer for further procession
        std::shared_ptr<GrayscaleImage> buf = std::shared_ptr<GrayscaleImage>(
            new GrayscaleImage(vwidth, vheight, currentTimestamp));
        memcpy(&buf.get()->data[0], cimg.GetData(), vwidth * vheight);

        _videoStream.push(buf);
    }
    _Camera.StopCapture();
    _Camera.Disconnect();

    return;
}

void Flea3CamThread::logCriticalError(FlyCapture2::Error e)
{
    char              logfilepathFull[256];
    std::stringstream str;
    SettingsIAC*      set      = SettingsIAC::getInstance();
    std::string       logdir   = set->logDirectory();
    std::string       shortmsg = "Short log: \n";
    str << "Error acquiring image. Printing full info and exiting. " << std::endl;
    str << "Type: " << e.GetType() << std::endl;
    str << "Description: " << e.GetDescription() << std::endl;
    str << "Filename: " << e.GetFilename() << std::endl;
    shortmsg = shortmsg + str.str();

    str << "Line: " << e.GetLine() << std::endl;
    str << "ErrorType: " << e.GetType() << std::endl;
    FlyCapture2::Error cause = e.GetCause();
    str << "Cause.Type: " << cause.GetType() << std::endl;
    str << "Cause.Description: " << cause.GetDescription() << std::endl;
    str << "Cause.Filename: " << cause.GetFilename() << std::endl;
    str << "Cause.Line: " << cause.GetLine() << std::endl;
    str << "Cause.ErrorType: " << cause.GetType() << std::endl;
    str << "Exit! " << std::endl;
    sprintf(logfilepathFull, logdir.c_str(), _videoStream.id);
    generateLog(logfilepathFull, str.str().c_str());
}

// We will use Format7 to set the video parameters instead of DCAM, so it becomes handy to print
// this info
void Flea3CamThread::PrintFormat7Capabilities(FlyCapture2::Format7Info fmt7Info)
{
    sendLogMessage(3,
                   "Max image pixels: " + QString::number(fmt7Info.maxWidth) + " x " +
                       QString::number(fmt7Info.maxHeight) + "\n" +
                       "Image Unit size: " + QString::number(fmt7Info.imageHStepSize) + " x " +
                       QString::number(fmt7Info.imageVStepSize) + "\n" +
                       "Offset Unit size: " + QString::number(fmt7Info.offsetHStepSize) + " x " +
                       QString::number(fmt7Info.offsetVStepSize) + "\n" +
                       "Pixel format bitfield: " + QString::number(fmt7Info.pixelFormatBitField));
}

// Just prints the camera's info
void Flea3CamThread::PrintCameraInfo(FlyCapture2::CameraInfo* pCamInfo)
{
    sendLogMessage(3,
                   QString() + "\n*** CAMERA INFORMATION ***\n" + "Serial number - " +
                       QString::number(pCamInfo->serialNumber) + "\n" + "Camera model - " +
                       QString(pCamInfo->modelName) + "\n" + "Camera vendor - " +
                       QString(pCamInfo->vendorName) + "\n" + "Sensor - " +
                       QString(pCamInfo->sensorInfo) + "\n" + "Resolution - " +
                       QString(pCamInfo->sensorResolution) + "\n" + "Firmware version - " +
                       QString(pCamInfo->firmwareVersion) + "\n" + "Firmware build time - " +
                       QString(pCamInfo->firmwareBuildTime) + "\n" + "\n");
}

bool Flea3CamThread::checkReturnCode(FlyCapture2::Error error)
{
    if (error != FlyCapture2::PGRERROR_OK)
    {
        sendLogMessage(1,
                       "Cam " + QString::fromStdString(_videoStream.id) + " : " +
                           error.GetDescription());
        return false;
    }
    return true;
}

void Flea3CamThread::sendLogMessage(int logLevel, QString message)
{
    emit logMessage(logLevel, "Cam " + QString::fromStdString(_videoStream.id) + " : " + message);
}

void Flea3CamThread::generateLog(QString path, QString message)
{
    boost::filesystem::create_directories({path.toStdString()});
    QString filename = (path + "log.txt");
    QFile   file(filename);
    file.open(QIODevice::Append);
    QTextStream stream(&file);
    stream << QString::fromStdString(getTimestamp()) << ": " << message << "\r\n";
    file.close();
}
