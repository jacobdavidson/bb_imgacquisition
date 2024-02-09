// SPDX-License-Identifier: GPL-3.0-or-later

#include "XimeaCamera.hpp"

#include <array>
#include <chrono>

#include "util/format.hpp"
#include "util/log.hpp"
#include "GrayscaleImage.hpp"

XimeaCamera::XimeaCamera(Config config, ImageStream imageStream)
: Camera(config, imageStream)
{
    initCamera();
    startCapture();
}

void XimeaCamera::initCamera()
{
    // Try settings that our mc124mg does not support.
    // This might become relevant with other XIMEA cams.
    // However, with our camera, it just displays warnings.
    const bool tryAllXimeaCamSettings = false;

    // Disable bandwith-measuring for Ximea cams.
    // According to docs, this can be wonky with multiple cameras.
    enforce(xiSetParamInt(0, XI_PRM_AUTO_BANDWIDTH_CALCULATION, XI_OFF),
            "xiSetParamInt XI_PRM_AUTO_BANDWIDTH_CALCULATION");

    if (_config.serial.empty())
    {
        throw std::runtime_error(fmt::format("{}: Camera serial empty", _imageStream.id));
    }

    DWORD numCameras;
    if (auto err = xiGetNumberDevices(&numCameras); err != XI_OK)
    {
        throw std::runtime_error("Could not enumerate cameras");
    }

    bool camFound = false;
    for (decltype(numCameras) i = 0; i < numCameras; ++i)
    {
        HANDLE cam;
        if (xiOpenDevice(i, &cam) != XI_OK)
        {
            // Maybe the cam has already been opened by another thread.
            continue;
        }

        char serial[20] = "";
        if (xiGetParamString(cam, XI_PRM_DEVICE_SN, serial, sizeof(serial)) == XI_OK)
        {
            if (_config.serial == serial)
            {
                camFound = true;
                _Camera  = cam;
                // Do NOT close the camera.
                break;
            }
        }
        else
        {
            logWarning("{}: Failed to get serial of camera #{}", _imageStream.id, i);
        }

        // Close the camera if mismatch.
        xiCloseDevice(cam);
    }

    if (!camFound)
    {
        throw std::runtime_error(
            fmt::format("{}: No camera matching serial: {}", _imageStream.id, _config.serial));
    }

    // Print some interesting meta data.

    // Temperature.
    if (tryAllXimeaCamSettings)
    {
        if (xiSetParamInt(_Camera, XI_PRM_TEMP_SELECTOR, XI_TEMP_INTERFACE_BOARD) == XI_OK)
        {
            float temperature = 0;
            if (xiGetParamFloat(_Camera, XI_PRM_TEMP, &temperature) == XI_OK)
            {
                logInfo("{}: Camera sensor temperature  at{:.5f} °C",
                        _imageStream.id,
                        temperature);
            }
            else
            {
                logInfo("{}: Failed to get camera sensor temperature", _imageStream.id);
            }
        }
        else
        {
            logInfo("{}: Camera sensor temperature unavailable", _imageStream.id);
        }
    }

    if (_config.params.throughput_limit)
    {
        // Set the bandwidth limit.
        enforce(xiSetParamInt(_Camera, XI_PRM_LIMIT_BANDWIDTH, *_config.params.throughput_limit),
                "xiSetParamInt XI_PRM_LIMIT_BANDWIDTH");
        enforce(xiSetParamInt(_Camera, XI_PRM_LIMIT_BANDWIDTH_MODE, XI_ON),
                "xiSetParamInt XI_PRM_LIMIT_BANDWIDTH_MODE");
    }

    // Set a format that corresponds to the Y in YUV.
    enforce(xiSetParamInt(_Camera, XI_PRM_IMAGE_DATA_FORMAT, XI_RAW8),
            "xiSetParamInt XI_PRM_IMAGE_DATA_FORMAT");

    // Deactivate downsampling.
    if (tryAllXimeaCamSettings)
    {
        enforce(xiSetParamInt(_Camera, XI_PRM_DOWNSAMPLING, 1),
                "xiSetParamInt XI_PRM_IMAGE_DATA_FORMAT");
    }

    // Select configured properties.

    enforce(xiSetParamInt(_Camera, XI_PRM_OFFSET_X, _config.params.offset_x),
            "xiSetParamInt XI_PRM_OFFSET_X");
    enforce(xiSetParamInt(_Camera, XI_PRM_OFFSET_Y, _config.params.offset_y),
            "xiSetParamInt XI_PRM_OFFSET_Y");
    enforce(xiSetParamInt(_Camera, XI_PRM_WIDTH, _config.params.width),
            "xiSetParamInt XI_PRM_WIDTH");
    enforce(xiSetParamInt(_Camera, XI_PRM_HEIGHT, _config.params.height),
            "xiSetParamInt XI_PRM_HEIGHT");

    {
        int offsetX;
        int offsetY;
        int width;
        int height;

        enforce(xiGetParamInt(_Camera, XI_PRM_OFFSET_X, &offsetX),
                "xiGetParamInt XI_PRM_OFFSET_X");
        enforce(xiGetParamInt(_Camera, XI_PRM_OFFSET_Y, &offsetY),
                "xiGetParamInt XI_PRM_OFFSET_Y");
        enforce(xiGetParamInt(_Camera, XI_PRM_WIDTH, &width), "xiGetParamInt XI_PRM_WIDTH");
        enforce(xiGetParamInt(_Camera, XI_PRM_HEIGHT, &height), "xiGetParamInt XI_PRM_HEIGHT");

        if (!(offsetX == _config.params.offset_x && offsetY == _config.params.offset_y &&
              width == _config.params.width && height == _config.params.height))
        {
            throw std::runtime_error(
                fmt::format("{}: Could not set camera region of intereset to ({},{}),{}x{}",
                            _imageStream.id,
                            _config.params.offset_x,
                            _config.params.offset_y,
                            _config.params.width,
                            _config.params.height));
        }
    }

    enforce(xiSetParamInt(_Camera, XI_PRM_TRG_SELECTOR, XI_TRG_SEL_FRAME_START),
            "xiSetParamInt XI_PRM_TRG_SELECTOR");

    if (auto* trigger = std::get_if<SoftwareTrigger>(&_config.params.trigger))
    {
        if (xiSetParamInt(_Camera, XI_PRM_ACQ_TIMING_MODE, XI_ACQ_TIMING_MODE_FRAME_RATE) == XI_OK)
        {
            xiSetParamFloat(_Camera, XI_PRM_FRAMERATE, trigger->framesPerSecond);
        }

        enforce(xiSetParamInt(_Camera, XI_PRM_TRG_SOURCE, XI_TRG_OFF),
                "xiSetParamInt XI_PRM_TRG_SOURCE");
    }
    else if (auto* trigger = std::get_if<HardwareTrigger>(&_config.params.trigger))
    {
        enforce(xiSetParamInt(_Camera, XI_PRM_GPI_SELECTOR, trigger->source),
                "xiSetParamInt XI_PRM_GPI_SELECTOR");
        enforce(xiSetParamInt(_Camera, XI_PRM_GPI_MODE, XI_GPI_TRIGGER),
                "xiSetParamInt XI_PRM_GPI_MODE");

        enforce(xiSetParamInt(_Camera, XI_PRM_TRG_SOURCE, XI_TRG_EDGE_RISING),
                "xiSetParamInt XI_PRM_TRG_SOURCE");
    }
    else
    {
        throw std::logic_error("Not implemented");
    }

    if (_config.params.blacklevel)
    {
        logWarning("{}: Blacklevel parameter not implemented for Basler cameras", _imageStream.id);
    }

    if (_config.params.exposure)
    {
        // Explicitly enforce 1 exposure per frame
        enforce(xiSetParamInt(_Camera, XI_PRM_EXPOSURE_BURST_COUNT, 1),
                "xiSetParamInt XI_PRM_EXPOSURE_BURST_COUNT");
    }

    if (_config.params.exposure && _config.params.gain)
    {
        const auto autoExposure = std::holds_alternative<Parameter_Auto>(*_config.params.exposure);
        const auto autoGain     = std::holds_alternative<Parameter_Auto>(*_config.params.exposure);

        if (autoExposure && autoGain)
        {
            enforce(xiSetParamInt(_Camera, XI_PRM_AEAG, 1), "xiSetParamInt XI_PRM_AEAG");
        }
        else if (!autoExposure && !autoGain)
        {
            enforce(xiSetParamInt(_Camera, XI_PRM_AEAG, 0), "xiSetParamInt XI_PRM_AEAG");

            enforce(xiSetParamInt(_Camera,
                                  XI_PRM_EXPOSURE,
                                  std::get<Parameter_Manual<float>>(*_config.params.exposure)),
                    "xiSetParamInt XI_PRM_EXPOSURE");

            enforce(xiSetParamInt(_Camera, XI_PRM_GAIN_SELECTOR, XI_GAIN_SELECTOR_ALL),
                    "xiSetParamInt XI_PRM_GAIN_SELECTOR");
            enforce(xiSetParamFloat(_Camera,
                                    XI_PRM_GAIN,
                                    std::get<Parameter_Manual<float>>(*_config.params.gain)),
                    "xiSetParamFloat XI_PRM_GAIN");
        }
        else
        {
            throw std::runtime_error(
                fmt::format("{}: Automatic gain enables automatic exposure and vice versa on "
                            "XIMEA cameras",
                            _imageStream.id));
        }
    }
    else if (_config.params.exposure)
    {
        enforce(xiSetParamInt(_Camera, XI_PRM_AEAG, 0), "xiSetParamInt XI_PRM_AEAG");

        enforce(xiSetParamInt(_Camera,
                              XI_PRM_EXPOSURE,
                              std::get<Parameter_Manual<float>>(*_config.params.exposure)),
                "xiSetParamInt XI_PRM_EXPOSURE");
    }
    else if (_config.params.gain)
    {
        enforce(xiSetParamInt(_Camera, XI_PRM_AEAG, 0), "xiSetParamInt XI_PRM_AEAG");

        enforce(xiSetParamInt(_Camera, XI_PRM_GAIN_SELECTOR, XI_GAIN_SELECTOR_ALL),
                "xiSetParamInt XI_PRM_GAIN_SELECTOR");
        enforce(xiSetParamFloat(_Camera,
                                XI_PRM_GAIN,
                                std::get<Parameter_Manual<float>>(*_config.params.gain)),
                "xiSetParamFloat XI_PRM_GAIN");
    }

    // The default is no buffering.
    if (_config.params.buffer_size)
    {
        const auto buffer_size = *_config.params.buffer_size;

        enforce(xiSetParamInt(_Camera, XI_PRM_BUFFERS_QUEUE_SIZE, buffer_size + 1),
                "xiSetParamInt XI_PRM_BUFFERS_QUEUE_SIZE");
        enforce(xiSetParamInt(_Camera, XI_PRM_RECENT_FRAME, buffer_size == 0 ? 1 : 0),
                "xiSetParamInt XI_PRM_RECENT_FRAME");
        if (buffer_size > 0)
        {
            enforce(xiSetParamInt(_Camera,
                                  XI_PRM_ACQ_BUFFER_SIZE,
                                  3 * (buffer_size + 1) * 3008 * 4112),
                    "xiSetParamInt XI_PRM_ACQ_BUFFER_SIZE");
        }
    }

    {
        int max_transport_buf_size{0};
        enforce(xiGetParamInt(_Camera,
                              XI_PRM_ACQ_TRANSPORT_BUFFER_SIZE XI_PRM_INFO_MAX,
                              &max_transport_buf_size),
                "xiGetParamInt XI_PRM_ACQ_TRANSPORT_BUFFER_SIZE XI_PRM_INFO_MAX");
        enforce(xiSetParamInt(_Camera, XI_PRM_ACQ_TRANSPORT_BUFFER_SIZE, max_transport_buf_size),
                "xiSetParamInt XI_PRM_ACQ_TRANSPORT_BUFFER_SIZE");
    }
}

void XimeaCamera::startCapture()
{
    enforce(xiStartAcquisition(_Camera), "xiStartAcquisition");
}

void XimeaCamera::run()
{
    using namespace std::chrono_literals;

    std::uint64_t lastImageSequenceNumber = 0;

    // The camera timestamp will be used to get a more accurate idea of when the image was taken.
    // Software hangups (e.g. short CPU spikes) can thus be mitigated.
    auto currCameraTime      = std::chrono::system_clock::time_point{};
    auto lastCameraTimestamp = 0us;

    // Preallocate image buffer on stack in order to save performance later.
    std::array<unsigned char, 3008 * 4112> imageBuffer;

    for (std::size_t loopCount = 0; !isInterruptionRequested(); loopCount += 1)
    {
        XI_IMG image;
        image.size    = sizeof(XI_IMG);
        image.bp      = static_cast<LPVOID>(&imageBuffer[0]);
        image.bp_size = imageBuffer.size();

        const auto begin = std::chrono::steady_clock::now();
        if (const auto returnCode = xiGetImage(_Camera, 1000 * 2, &image); returnCode != XI_OK)
        {
            throw std::runtime_error(
                fmt::format("{}: Failed to grab camera image: {}", _imageStream.id, returnCode));
        }
        const auto end = std::chrono::steady_clock::now();

        const auto currWallClockTime = std::chrono::system_clock::now();
        // NOTE: Camera time is cyclic (if available):
        //   xiMU: not implemented
        //   xiQ, xiD: 305 hours, recorded at start of data readout
        //   xiC,xiB,xiT,xiX: 2339 years, recorded at start of exposure
        const auto currCameraTimestamp = std::chrono::microseconds(image.tsUSec) +
                                         std::chrono::seconds(image.tsSec);

        // Image sequence sanity check.
        if (lastImageSequenceNumber != 0 && image.nframe != lastImageSequenceNumber + 1)
        {
            logWarning("{}: Camera lost frame: This frame: #{} last frame: #{} timestamp: {:e.6}",
                       _imageStream.id,
                       image.nframe,
                       lastImageSequenceNumber,
                       currWallClockTime);

            for (auto& what : std::map<std::string, int>{
                     {"Transport layer loss: ", XI_CNT_SEL_TRANSPORT_SKIPPED_FRAMES},
                     {"API layer loss: ", XI_CNT_SEL_API_SKIPPED_FRAMES}})
            {
                xiSetParamInt(_Camera, XI_PRM_COUNTER_SELECTOR, what.second);
                int result = 0;
                xiGetParamInt(_Camera, XI_PRM_COUNTER_VALUE, &result);
                logWarning("{}: {}: {}", _imageStream.id, what.first, result);
            }
        }
        lastImageSequenceNumber = image.nframe;

        // NOTE: If we detected camera model we could calculate the timestamp difference modulo
        //       timer cycle instead.
        if (lastCameraTimestamp == 0us || lastCameraTimestamp > currCameraTimestamp)
        {
            if (lastCameraTimestamp != 0us && currCameraTime > currWallClockTime && loopCount > 10)
            {
                logWarning(
                    "{}: Camera clock faster than wall time. Last camera time: {:e.6} wall "
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
            assert(elapsed >= 0us);
            currCameraTime += elapsed;
        }
        lastCameraTimestamp = currCameraTimestamp;

        // Skip the first X images to allow the camera buffer to be flushed.
        if (loopCount < 10)
        {
            continue;
        }

        if (const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                                        begin);
            duration > 333333us)
        {
            logWarning("{}: Processing time too long: {}", _imageStream.id, duration);
        }

        auto cvImage = transform(static_cast<int>(image.width),
                                 static_cast<int>(image.height),
                                 image.bp);

        auto capturedImage = GrayscaleImage(cvImage.cols, cvImage.rows, currCameraTime);
        std::memcpy(&capturedImage.data[0], cvImage.data, cvImage.cols * cvImage.rows);

        _imageStream.push(capturedImage);
        emit imageCaptured(capturedImage);
    }

    enforce(xiStopAcquisition(_Camera), "xiStopAcquisition");
    enforce(xiCloseDevice(_Camera), "xiCloseDevice");
}

void XimeaCamera::enforce(XI_RETURN errorCode, const std::string& operation)
{
    if (errorCode != XI_OK)
    {
        throw std::runtime_error(
            fmt::format("{}: Camera error: {}: {}", _imageStream.id, operation, errorCode));
    }
}
