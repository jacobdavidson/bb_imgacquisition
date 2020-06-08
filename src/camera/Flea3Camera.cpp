// SPDX-License-Identifier: GPL-3.0-or-later

#include "Flea3Camera.hpp"

#include <chrono>

#include "util/format.hpp"
#include "util/log.hpp"
#include "util/type_traits.hpp"
#include "GrayscaleImage.hpp"

Flea3Camera::Flea3Camera(Config config, ImageStream imageStream)
: Camera(config, imageStream)
{
    initCamera();
    startCapture();
}

void Flea3Camera::initCamera()
{
    // SET VIDEO MODE HERE!!!
    FlyCapture2::Format7Info fmt7Info;

    const FlyCapture2::Mode        fmt7Mode   = FlyCapture2::MODE_10;
    const FlyCapture2::PixelFormat fmt7PixFmt = FlyCapture2::PIXEL_FORMAT_RAW8;

    FlyCapture2::Format7ImageSettings fmt7ImageSettings;

    fmt7ImageSettings.mode        = fmt7Mode;
    fmt7ImageSettings.offsetX     = _config.params.offset_x;
    fmt7ImageSettings.offsetY     = _config.params.offset_y;
    fmt7ImageSettings.width       = _config.params.width;
    fmt7ImageSettings.height      = _config.params.height;
    fmt7ImageSettings.pixelFormat = fmt7PixFmt;

    FlyCapture2::Format7PacketInfo fmt7PacketInfo;
    // fmt7PacketInfo.recommendedBytesPerPacket = 5040;

    FlyCapture2::BusManager busMgr;
    FlyCapture2::PGRGuid    guid;
    FlyCapture2::CameraInfo camInfo;

    FlyCapture2::FC2Config BufferFrame;

    FlyCapture2::EmbeddedImageInfo EmbeddedInfo;

    // Properties to be modified

    FlyCapture2::Property blacklevel;
    blacklevel.type = FlyCapture2::BRIGHTNESS;

    FlyCapture2::Property exposure;
    exposure.type = FlyCapture2::SHUTTER;

    FlyCapture2::Property gain;
    gain.type = FlyCapture2::GAIN;

    FlyCapture2::Property frmRate;
    frmRate.type = FlyCapture2::FRAME_RATE;

    FlyCapture2::Property wBalance;
    wBalance.type = FlyCapture2::WHITE_BALANCE;

    bool supported;

    unsigned int numCameras;
    if (auto err = busMgr.GetNumOfCameras(&numCameras); err != FlyCapture2::PGRERROR_OK)
    {
        throw std::runtime_error(fmt::format("{}: Could not enumerate cameras: {}",
                                             _imageStream.id,
                                             err.GetDescription()));
    }

    bool         camFound = false;
    unsigned int camIndex;
    unsigned int serial;
    for (decltype(numCameras) i = 0; i < numCameras; i++)
    {
        if (auto err = busMgr.GetCameraSerialNumberFromIndex(i, &serial);
            err != FlyCapture2::PGRERROR_OK)
        {
            throw std::runtime_error(fmt::format("{}: Could not read camera serial: {}",
                                                 _imageStream.id,
                                                 err.GetDescription()));
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
        throw std::runtime_error(
            fmt::format("{}: No camera matching serial: {}", _imageStream.id, _config.serial));
    }

    // Gets the PGRGuid from the camera
    enforce(busMgr.GetCameraFromIndex(camIndex, &guid));

    // Connect to camera
    enforce(_Camera.Connect(&guid));

    // Get the camera information
    enforce(_Camera.GetCameraInfo(&camInfo));

    PrintCameraInfo(&camInfo); // camera information is printed

    /////////////////// ALL THE PROCESS WITH FORMAT 7 ////////////////////////////////

    // Query for available Format 7 modes
    enforce(_Camera.GetFormat7Info(&fmt7Info, &supported));

    // Print the camera capabilities for fmt7
    PrintFormat7Capabilities(fmt7Info);

    // Validate the settings to make sure that they are valid
    enforce(_Camera.ValidateFormat7Settings(&fmt7ImageSettings, &supported, &fmt7PacketInfo));

    if (!supported)
    {
        throw std::runtime_error(fmt::format("{}: Invalid Format7 settings ", _imageStream.id));
    }

    // Set the settings to the camera
    enforce(_Camera.SetFormat7Configuration(&fmt7ImageSettings,
                                            fmt7PacketInfo.recommendedBytesPerPacket));

    /////////////////////////////// ENDS PROCESS WITH FORMAT 7
    /////////////////////////////////////////////

    /////////////////// ALL THE PROCESS WITH FC2Config  ////////////////////////////////

    if (_config.params.buffer_size)
    {
        // Grab the current configuration from the camera in ss_BufferFrame
        enforce(_Camera.GetConfiguration(&BufferFrame));

        // Modify the maximum number of frames to be buffered and send it back to the camera
        BufferFrame.numBuffers = *_config.params.buffer_size;
        BufferFrame.grabMode   = FlyCapture2::BUFFER_FRAMES;

        enforce(_Camera.SetConfiguration(&BufferFrame));
    }

    /////////////////// ENDS PROCESS WITH FC2Config  ////////////////////////////////

    /////////////////// ALL THE PROCESS WITH EMBEDDEDIMAGEINFO  ////////////////////////////////

    // Get the info of the image in ss_EmbeddedInfo
    enforce(_Camera.GetEmbeddedImageInfo(&EmbeddedInfo));

    // Again modify a couple of parameters and send them back to the camera
    if (EmbeddedInfo.timestamp.available == true)
    {
        // if possible activates timestamp
        EmbeddedInfo.timestamp.onOff = true;
    }
    else
    {
        logInfo("{}: Embedded camera timestamp is not available", _imageStream.id);
    }

    if (EmbeddedInfo.frameCounter.available == true)
    {
        // if possible activates frame counter
        EmbeddedInfo.frameCounter.onOff = true;
    }
    else
    {
        logInfo("{}: Embedded camera frame counter is not available", _imageStream.id);
    }

    enforce(_Camera.SetEmbeddedImageInfo(&EmbeddedInfo));

    /////////////////// ENDS PROCESS WITH EMBEDDEDIMAGEINFO  ////////////////////////////////

    /////////////////// ALL THE PROCESS WITH PROPERTIES  ////////////////////////////////

    //-------------------- BLACKLEVEL STARTS        -----------------------------------

    if (_config.params.blacklevel)
    {
        enforce(_Camera.GetProperty(&blacklevel));

        blacklevel.onOff = true;
        std::visit(
            [&blacklevel](auto&& value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, Parameter_Auto>)
                    blacklevel.autoManualMode = true;
                else if constexpr (std::is_same_v<T, Parameter_Manual<float>>)
                {
                    blacklevel.autoManualMode = false;
                    blacklevel.absValue       = value;
                }
                else
                    static_assert(false_type<T>::value);
            },
            *_config.params.blacklevel);

        enforce(_Camera.SetProperty(&blacklevel));
    }

    enforce(_Camera.GetProperty(&blacklevel));

    logInfo("{}: Blacklevel parameter is {} and {:.2f}",
            _imageStream.id,
            blacklevel.onOff ? "on" : "off",
            blacklevel.absValue);

    //-------------------- BLACKLEVEL ENDS          -----------------------------------

    //-------------------- EXPOSURE STARTS          -----------------------------------
    if (_config.params.exposure)
    {
        enforce(_Camera.GetProperty(&exposure));

        exposure.onOff = true;
        std::visit(
            [&exposure](auto&& value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, Parameter_Auto>)
                    exposure.autoManualMode = true;
                else if constexpr (std::is_same_v<T, Parameter_Manual<float>>)
                {
                    exposure.autoManualMode = false;
                    exposure.absValue       = value;
                }
                else
                    static_assert(false_type<T>::value);
            },
            *_config.params.exposure);

        enforce(_Camera.SetProperty(&exposure));
    }
    enforce(_Camera.GetProperty(&exposure));

    logInfo("{}: Exposure parameter is {} and {}",
            _imageStream.id,
            exposure.onOff ? "on" : "off",
            exposure.autoManualMode ? "auto" : "manual");

    //-------------------- EXPOSURE ENDS -----------------------------------

    //-------------------- GAIN STARTS              -----------------------------------
    if (_config.params.gain)
    {
        enforce(_Camera.GetProperty(&gain));

        gain.onOff = true;
        std::visit(
            [&gain](auto&& value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, Parameter_Auto>)
                    gain.autoManualMode = true;
                else if constexpr (std::is_same_v<T, Parameter_Manual<float>>)
                {
                    gain.autoManualMode = false;
                    gain.absValue       = value;
                }
                else
                    static_assert(false_type<T>::value);
            },
            *_config.params.gain);

        enforce(_Camera.SetProperty(&gain));
    }
    enforce(_Camera.GetProperty(&gain));

    logInfo("{}: Gain parameter is {} and {}",
            _imageStream.id,
            gain.onOff ? "on" : "off",
            gain.autoManualMode ? "auto" : "manual");
    //-------------------- GAIN ENDS                -----------------------------------

    //-------------------- TRIGGER MODE STARTS      -----------------------------------

    if (std::holds_alternative<HardwareTrigger>(_config.params.trigger))
    {
        // Get current trigger settings
        FlyCapture2::TriggerMode triggerMode;
        enforce(_Camera.GetTriggerMode(&triggerMode));

        // Set camera to trigger mode 0
        triggerMode.onOff     = true;
        triggerMode.mode      = 0;
        triggerMode.parameter = 0;

        // Triggering the camera externally using source 0.
        triggerMode.source = std::get<HardwareTrigger>(_config.params.trigger).source;

        enforce(_Camera.SetTriggerMode(&triggerMode));
    }

    //-------------------- TRIGGER MODE ENDS        -----------------------------------

    //-------------------- FRAME RATE STARTS        -----------------------------------

    if (std::holds_alternative<SoftwareTrigger>(_config.params.trigger))
    {
        enforce(_Camera.GetProperty(&frmRate));

        frmRate.absControl     = true;
        frmRate.onOff          = true;
        frmRate.autoManualMode = false;
        frmRate.absValue       = std::get<SoftwareTrigger>(_config.params.trigger).framesPerSecond;

        enforce(_Camera.SetProperty(&frmRate));

        enforce(_Camera.GetProperty(&frmRate));

        logInfo("{}: Frame rate is {:.2f} fps", _imageStream.id, frmRate.absValue);
    }

    //-------------------- FRAME RATE ENDS          -----------------------------------

    /////////////////// ALL THE PROCESS WITH EMBEDDEDIMAGEINFO  ////////////////////////////////
    // Saves the configuration to memory channel 2
    enforce(_Camera.SaveToMemoryChannel(2));
}

// This function starts the streaming from the camera
void Flea3Camera::startCapture()
{
    // Start isochronous image capture
    enforce(_Camera.StartCapture());
}

void Flea3Camera::run()
{
    using namespace std::chrono_literals;

    // The first frame usually contains weird metadata. So we grab it and discard it.
    {
        FlyCapture2::Image cimg;
        _Camera.RetrieveBuffer(&cimg);
        _Camera.RetrieveBuffer(&cimg);
    }

    while (!isInterruptionRequested())
    {
        FlyCapture2::Image cimg;

        const auto begin = std::chrono::steady_clock::now();
        if (const auto err = _Camera.RetrieveBuffer(&cimg); err != FlyCapture2::PGRERROR_OK)
        {
            throw std::runtime_error(fmt::format("{}: Failed to grab camera image: {}",
                                                 _imageStream.id,
                                                 err.GetDescription()));
        }
        const auto end = std::chrono::steady_clock::now();

        // Get the timestamp
        const auto currWallClockTime = std::chrono::system_clock::now();

        if (const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                                        begin);
            duration > 400ms)
        {
            logWarning("{}: Processing time too long: {}", _imageStream.id, duration);
        }

        auto cvImage = transform(static_cast<int>(cimg.GetCols()),
                                 static_cast<int>(cimg.GetRows()),
                                 cimg.GetData());

        auto capturedImage = GrayscaleImage(cvImage.cols, cvImage.rows, currWallClockTime);
        std::memcpy(&capturedImage.data[0], cvImage.data, cvImage.cols * cvImage.rows);

        _imageStream.push(capturedImage);
        emit imageCaptured(capturedImage);
    }

    _Camera.StopCapture();
    _Camera.Disconnect();
}

// We will use Format7 to set the video parameters instead of DCAM, so it becomes handy to print
// this info
void Flea3Camera::PrintFormat7Capabilities(FlyCapture2::Format7Info fmt7Info)
{
    logInfo("{}: Max image pixels: {}x{}", _imageStream.id, fmt7Info.maxWidth, fmt7Info.maxHeight);
    logInfo("{}: Image Unit size: {}x{}",
            _imageStream.id,
            fmt7Info.imageHStepSize,
            fmt7Info.imageVStepSize);
    logInfo("{}: Offset Unit size: {}x{}",
            _imageStream.id,
            fmt7Info.offsetHStepSize,
            fmt7Info.offsetVStepSize);
    logInfo("{}: Pixel format bitfield: {}", _imageStream.id, fmt7Info.pixelFormatBitField);
}

// Just prints the camera's info
void Flea3Camera::PrintCameraInfo(FlyCapture2::CameraInfo* pCamInfo)
{
    logInfo("{}: Camera serial: {}", _imageStream.id, pCamInfo->serialNumber);
    logInfo("{}: Camera model: {}", _imageStream.id, std::string_view(pCamInfo->modelName));
    logInfo("{}: Camera vendor: {}", _imageStream.id, std::string_view(pCamInfo->vendorName));
    logInfo("{}: Camera sensor: {}", _imageStream.id, std::string_view(pCamInfo->sensorInfo));
    logInfo("{}: Camera resolution: {}",
            _imageStream.id,
            std::string_view(pCamInfo->sensorResolution));
    logInfo("{}: Camera firmware version: {}",
            _imageStream.id,
            std::string_view(pCamInfo->firmwareVersion));
    logInfo("{}: Camera firmware build time: {}",
            _imageStream.id,
            std::string_view(pCamInfo->firmwareBuildTime));
}

void Flea3Camera::enforce(FlyCapture2::Error error)
{
    if (error != FlyCapture2::PGRERROR_OK)
    {
        throw std::runtime_error(
            fmt::format("{}: Camera error: {}", _imageStream.id, error.GetDescription()));
    }
}
