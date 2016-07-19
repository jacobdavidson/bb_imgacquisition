#include "Flea3CamThread.h"

#include <cstdlib>
#include <iostream>

#include <qdir.h>       //QT stuff
#include <qtextstream.h>

#include "FlyCapture2.h"
#include "Watchdog.h"
#include "settings/Settings.h"
#include "settings/utility.h"
#include "ImageAnalysis.h"
#include <sstream> //stringstreams

#include <ctime> //get time
#include <time.h>

#if __linux__
#include <unistd.h> //sleep
#include <sys/time.h>
#else
#include <stdint.h>
#endif

#if HALIDE
#include "halideYuv420Conv.h"
#include "Halide.h"
//On Windows this might conflict with predefs
#undef user_error
#endif

#include <opencv2/opencv.hpp>
using namespace cv;

//Flea3CamThread constructor
Flea3CamThread::Flea3CamThread() {
    _initialized = false;
}

//Flea3CamThread constructor
Flea3CamThread::~Flea3CamThread() {

}

//this function reads the data input vector
bool Flea3CamThread::initialize(unsigned int id,
                                beeCompress::MutexBuffer *pBuffer,
                                beeCompress::MutexBuffer *pSharedMemBuffer, CalibrationInfo *calib,
                                Watchdog *dog) {
    _SharedMemBuffer = pSharedMemBuffer;
    _Buffer = pBuffer;
    _ID = id;
    _HWID = -1;
    _Calibration = calib;
    _initialized = false;
    _Dog = dog;

    if (initCamera()) {
        std::cout << "Starting capture on camera " << id << std::endl;
        _initialized = startCapture();
        std::cout << "Done starting capture." << std::endl;
    } else {
        std::cerr << "Error on camera init " << id << std::endl;
        return false;
    }

    return _initialized;
}

bool Flea3CamThread::initCamera() {

    SettingsIAC *set = SettingsIAC::getInstance();
    EncoderQualityConfig cfg = set->getBufferConf(_ID, 0);

    // SET VIDEO MODE HERE!!!
    Format7Info fmt7Info;

    const Mode fmt7Mode = MODE_10;
    const PixelFormat fmt7PixFmt = PIXEL_FORMAT_RAW8;

    Format7ImageSettings fmt7ImageSettings;

    fmt7ImageSettings.mode = fmt7Mode;
    fmt7ImageSettings.offsetX = 0;
    fmt7ImageSettings.offsetY = 0;
    fmt7ImageSettings.width = cfg.width;
    fmt7ImageSettings.height = cfg.height;
    fmt7ImageSettings.pixelFormat = fmt7PixFmt;

    Format7PacketInfo fmt7PacketInfo;
    //fmt7PacketInfo.recommendedBytesPerPacket = 5040;

    BusManager busMgr;
    PGRGuid guid;
    CameraInfo camInfo;
    unsigned int snum;

    FC2Config BufferFrame;

    EmbeddedImageInfo EmbeddedInfo;

    // Properties to be modified

    Property brightness;
    brightness.type = BRIGHTNESS;

    Property exposure;
    exposure.type = AUTO_EXPOSURE;

    Property shutter;
    shutter.type = SHUTTER;

    Property frmRate;
    frmRate.type = FRAME_RATE;

    Property gain;
    gain.type = GAIN;

    Property wBalance;
    wBalance.type = WHITE_BALANCE;

    bool supported;

    Error error;

    //Find hardware ID to serial number
    for (int i = 0; i < 4; i++) {
        unsigned int serial;
        Error error = busMgr.GetCameraSerialNumberFromIndex(i, &serial);
        if (error == PGRERROR_OK && serial == cfg.serial) {
            _HWID = i;
        }
    }

    if (_HWID == -1) {
        return false;
    }

    // Gets the Camera serial number
    if (!checkReturnCode(busMgr.GetCameraSerialNumberFromIndex(_HWID, &snum))) {
        return false;
    }

    sendLogMessage(3,
                   "Camera " + QString::number(_HWID) + "(HWID) "
                   + QString::number(_ID) + "(SWID) Serial Number: "
                   + QString::number(snum));   // serial number is printed

    // Gets the PGRGuid from the camera
    if (!checkReturnCode(busMgr.GetCameraFromIndex(_HWID, &guid))) {
        return false;
    }

    // Connect to camera
    if (!checkReturnCode(_Camera.Connect(&guid))) {
        return false;
    }

    // Get the camera information
    if (!checkReturnCode(_Camera.GetCameraInfo(&camInfo))) {
        return false;
    }

    PrintCameraInfo(&camInfo);  // camera information is printed

    /////////////////// ALL THE PROCESS WITH FORMAT 7 ////////////////////////////////

    // Query for available Format 7 modes
    if (!checkReturnCode(_Camera.GetFormat7Info(&fmt7Info, &supported))) {
        return false;
    }

    // Print the camera capabilities for fmt7
    PrintFormat7Capabilities(fmt7Info);

    // Validate the settings to make sure that they are valid
    if (!checkReturnCode(
                _Camera.ValidateFormat7Settings(&fmt7ImageSettings, &supported,
                        &fmt7PacketInfo))) {
        return false;
    }

    if (!supported) {
        // Settings are not valid
        sendLogMessage(1, "Format7 settings are not valid");
        return false;
    }

    // Set the settings to the camera
    if (!checkReturnCode(
                _Camera.SetFormat7Configuration(&fmt7ImageSettings,
                        fmt7PacketInfo.recommendedBytesPerPacket))) {
        return false;
    }

    /////////////////////////////// ENDS PROCESS WITH FORMAT 7 //////////////////////////////////////////

    /////////////////// ALL THE PROCESS WITH FC2Config  ////////////////////////////////

    // Grab the current configuration from the camera in ss_BufferFrame
    if (!checkReturnCode(_Camera.GetConfiguration(&BufferFrame))) {
        return false;
    }
    // Modify the maximum number of frames to be buffered and send it back to the camera
    BufferFrame.numBuffers = cfg.hwbuffersize; //default is 20
    BufferFrame.grabMode = BUFFER_FRAMES;

    //TODO: Re-enable this, if it happens to work some day
    // Exits if the configuration is not supported
    if (!checkReturnCode(_Camera.SetConfiguration(&BufferFrame))) {
        return false;
    }

    /////////////////// ENDS PROCESS WITH FC2Config  ////////////////////////////////

    /////////////////// ALL THE PROCESS WITH EMBEDDEDIMAGEINFO  ////////////////////////////////

    // Get the info of the image in ss_EmbeddedInfo
    if (!checkReturnCode(_Camera.GetEmbeddedImageInfo(&EmbeddedInfo))) {
        return false;
    }

    // Again modify a couple of parameters and send them back to the camera
    if (EmbeddedInfo.timestamp.available == true) {
        // if possible activates timestamp
        EmbeddedInfo.timestamp.onOff = true;
    } else {
        sendLogMessage(3, "Timestamp is not available!");
    }

    if (EmbeddedInfo.frameCounter.available == true) {
        // if possible activates frame counter
        EmbeddedInfo.frameCounter.onOff = true;
    } else {
        sendLogMessage(3, "Framecounter is not avalable!");
    }

    if (!checkReturnCode(_Camera.SetEmbeddedImageInfo(&EmbeddedInfo))) {
        return false;
    }

    /////////////////// ENDS PROCESS WITH EMBEDDEDIMAGEINFO  ////////////////////////////////

    /////////////////// ALL THE PROCESS WITH PROPERTIES  ////////////////////////////////

    //-------------------- BRIGHTNESS STARTS        -----------------------------------
    if (!checkReturnCode(_Camera.GetProperty(&brightness))) {
        return false;
    }

    brightness.onOff = cfg.brightnessonoff;     //default is true
    brightness.autoManualMode = cfg.brightnessauto;     //default is false
    brightness.absValue = cfg.brightness;           //default is 0

    if (!checkReturnCode(_Camera.SetProperty(&brightness))) {
        return false;
    }

    if (!checkReturnCode(_Camera.GetProperty(&brightness))) {
        return false;
    }

    sendLogMessage(3,
                   "Brightness Parameter is: "
                   + QString().sprintf("%s and %.2f",
                                       brightness.onOff ? "On" : "Off",
                                       brightness.absValue));

    //-------------------- BROGHTNESS ENDS          -----------------------------------

    //-------------------- EXPOSURE STARTS          -----------------------------------
    if (!checkReturnCode(_Camera.GetProperty(&exposure))) {
        return false;
    }

    exposure.onOff = cfg.exposureonoff;     //default is false
    exposure.autoManualMode = cfg.exposureauto;         //default is true

    if (!checkReturnCode(_Camera.SetProperty(&exposure))) {
        return false;
    }

    if (!checkReturnCode(_Camera.GetProperty(&exposure))) {
        return false;
    }

    sendLogMessage(3,
                   "Exposure Parameter is: "
                   + QString().sprintf("%s and %s",
                                       exposure.onOff ? "On" : "Off",
                                       exposure.autoManualMode ? "Auto" : "Manual"));

    //-------------------- EXPOSURE ENDS -----------------------------------

    //-------------------- SHUTTER STARTS           -----------------------------------
    if (!checkReturnCode(_Camera.GetProperty(&shutter))) {
        return false;
    }

    shutter.onOff = cfg.shutteronoff;       //default is true
    shutter.autoManualMode = cfg.shutterauto;           //default is false
    shutter.absValue = cfg.shutter;                 //default is 40

    if (!checkReturnCode(_Camera.SetProperty(&shutter))) {
        return false;
    }

    if (!checkReturnCode(_Camera.GetProperty(&shutter))) {
        return false;
    }

    sendLogMessage(3,
                   "New shutter parameter is: "
                   + QString().sprintf("%s and %.2f ms",
                                       shutter.autoManualMode ? "Auto" : "Manual",
                                       shutter.absValue));
    //-------------------- SHUTTER ENDS             -----------------------------------

    //-------------------- FRAME RATE STARTS        -----------------------------------
    if (!checkReturnCode(_Camera.GetProperty(&frmRate))) {
        return false;
    }

    frmRate.absControl = true;
    frmRate.onOff = true;
    frmRate.autoManualMode = false;
    frmRate.absValue = cfg.fps;

    if (!checkReturnCode(_Camera.SetProperty(&frmRate))) {
        return false;
    }

    if (!checkReturnCode(_Camera.GetProperty(&frmRate))) {
        return false;
    }

    sendLogMessage(3,
                   "New frame rate is " + QString().sprintf("%.2f", frmRate.absValue)
                   + " fps");

    //-------------------- FRAME RATE ENDS          -----------------------------------

    //-------------------- GAIN STARTS              -----------------------------------
    if (!checkReturnCode(_Camera.GetProperty(&gain))) {
        return false;
    }

    gain.onOff = cfg.gainonoff;     //default is true
    gain.autoManualMode = cfg.gainauto;     //default is false
    gain.absValue = cfg.gain;       //default is 0

    if (!checkReturnCode(_Camera.SetProperty(&gain))) {
        return false;
    }

    if (!checkReturnCode(_Camera.GetProperty(&gain))) {
        return false;
    }

    sendLogMessage(3,
                   "New gain parameter is: "
                   + QString().sprintf("%s",
                                       gain.autoManualMode ? "Auto" : "Manual"));
    //-------------------- GAIN ENDS                -----------------------------------

    //-------------------- WHITE BALANCE STARTS     -----------------------------------
    if (!checkReturnCode(_Camera.GetProperty(&wBalance))) {
        return false;
    }

    wBalance.onOff = cfg.whitebalance;  //default is false

    if (!checkReturnCode(_Camera.SetProperty(&wBalance))) {
        return false;
    }

    if (!checkReturnCode(_Camera.GetProperty(&wBalance))) {
        return false;
    }

    sendLogMessage(3,
                   "New White Balance parameter is: "
                   + QString().sprintf("%s", wBalance.onOff ? "On" : "Off"));
    //-------------------- WHITE BALANCE ENDS       -----------------------------------

    //-------------------- TRIGGER MODE STARTS      -----------------------------------

    // Get current trigger settings
    TriggerMode triggerMode;
    if (!checkReturnCode(_Camera.GetTriggerMode(&triggerMode))) {
        return false;
    }

    // Set camera to trigger mode 0
    triggerMode.onOff = cfg.hwtrigger;
    triggerMode.mode = 0;
    triggerMode.parameter = 0;

    // Triggering the camera externally using source 0.
    triggerMode.source = 0;

    if (!checkReturnCode(_Camera.SetTriggerMode(&triggerMode))) {
        return false;
    }

    //-------------------- TRIGGER MODE ENDS        -----------------------------------

    /////////////////// ALL THE PROCESS WITH EMBEDDEDIMAGEINFO  ////////////////////////////////
    //Saves the configuration to memory channel 2
    if (!checkReturnCode(_Camera.SaveToMemoryChannel(2))) {
        return false;
    }

    return true;
}

// This function starts the streaming from the camera
bool Flea3CamThread::startCapture() {
    // Start isochronous image capture
    return checkReturnCode(_Camera.StartCapture());
}

int flycapTo420NoHalide(uint8_t *yuvInput, FlyCapture2::Image *img) {
    /*  See: http://stackoverflow.com/questions/8349352/how-to-encode-grayscale-video-streams-with-ffmpeg */

    int bytesRead = 0;
    unsigned char *prtM = img->GetData();
    unsigned int rows, cols, stride;
    FlyCapture2::PixelFormat pixFmt;
    FlyCapture2::BayerTileFormat bayerFormat;
    img->GetDimensions(&rows, &cols, &stride, &pixFmt, &bayerFormat);

    //Being lazy pays: conversion in 58.22ms (don't set all channels - Cb and Cr are a waste of time)
    unsigned int x = 0, y = 0;
    for (y = 0; y < rows; y++) {
        for (x = 0; x < cols; x++) {
            yuvInput[y * cols + x] = (uint8_t)(0.895 * (*prtM) + 16);
            prtM++;
            bytesRead++;
        }
    }

    return bytesRead;
}

int flycapTo420(uint8_t *outputImage, FlyCapture2::Image *inputImage) {
    /*  See: http://stackoverflow.com/questions/8349352/how-to-encode-grayscale-video-streams-with-ffmpeg */

    int bytesRead;
#if HALIDE
    unsigned long long time, time2;
    unsigned char *prtM = inputImage->GetData();
    unsigned int rows,cols,stride;
    FlyCapture2::PixelFormat pixFmt;
    FlyCapture2::BayerTileFormat bayerFormat;
    inputImage->GetDimensions(&rows,&cols,&stride,&pixFmt,&bayerFormat);

    //Precompiled code gets run in 17.88ms. BAM!
    buffer_t input_buf = {0}, output_buf = {0};

    // The host pointers point to the start of the image data:
    input_buf.host = prtM;
    output_buf.host = outputImage;
    //See the halide tutorial how to set these.
    input_buf.stride[0] = output_buf.stride[0] = 1;
    input_buf.stride[1] = output_buf.stride[1] = stride;
    input_buf.extent[0] = output_buf.extent[0] = cols;
    input_buf.extent[1] = output_buf.extent[1] = rows;
    input_buf.elem_size = output_buf.elem_size = 1;

    //do it
    int error = halideYuv420Conv(&input_buf, &output_buf);
    bytesRead=rows*cols*3;//fool the system. We never read/written or set Cb and Cr
#else
    return flycapTo420NoHalide(outputImage, inputImage);
#endif

    return bytesRead;
}

void analyzeImage(int camid, FlyCapture2::Image *cimg, cv::Mat *ref,
                  CalibrationInfo *c) {
    beeCompress::ImageAnalysis ia("", NULL);

    //Create CV Matrix and make it use the flycap image ptr
    unsigned char *prtM = cimg->GetData();
    unsigned int rows, cols, stride;
    FlyCapture2::PixelFormat pixFmt;
    FlyCapture2::BayerTileFormat bayerFormat;
    cimg->GetDimensions(&rows, &cols, &stride, &pixFmt, &bayerFormat);
    cv::Mat mat(rows, cols, cv::DataType<uint8_t>::type);
    mat.data = prtM;

    //Analyze image quality
    double smd = ia.sumModulusDifference(&mat);
    double variance = ia.getVariance(mat);
    double contrast = ia.avgHistDifference(*ref, mat);
    double noise = ia.noiseEstimate(mat);

    //Write results to mutex datastructure
    c->dataAccess.lock();
    c->calibrationData[camid][0] = smd;
    c->calibrationData[camid][1] = variance;
    c->calibrationData[camid][2] = contrast;
    c->calibrationData[camid][3] = noise;
    c->dataAccess.unlock();

    //printf("Cam %d: %f,\t%f,\t%f,\t%f,\t%f\n",camid,smd,variance,contrast,noise);
}

//this is what the function does with the information set in configure
void Flea3CamThread::run() {
    struct tm *timeinfo;
    char timeresult[32];
    char logfilepathFull[256];

    SettingsIAC *set = SettingsIAC::getInstance();
    EncoderQualityConfig cfg = set->getBufferConf(_ID, 0);
    std::string logdir = set->getValueOfParam<std::string>(
                             IMACQUISITION::LOGDIR);

    int vwidth = cfg.width;
    int vheight = cfg.height;
    timeresult[14] = 0;
    int cont = 0;
    int loopCount = 0;
    BusManager busMgr;
    PGRGuid guid;

    ////////////////////////WINDOWS/////////////////////
    int oldTime = 0;
    unsigned int oldTimeUs = 1000000;
    unsigned int difTimeStampUs;
    ////////////////////////////////////////////////////

    ////////////////////////Logging and timekeeping/////
    std::chrono::steady_clock::time_point begin =
        std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now();
    sprintf(logfilepathFull, logdir.c_str(), _ID);
    ////////////////////////////////////////////////////

    ////////////////////////LINUX/////////////////////
#ifdef __linux__
    //Timestamp housekeeping
    time_t rawtime;
    struct timeval tv;
    struct timezone tz;
    int cs;
    long int startTime;

    gettimeofday(&tv, &tz);
    startTime = tv.tv_sec; //Note: using this method the start might be shifted by <1sec
    //The first frame usually contains weird metadata. So we grab it and discard it.
    {
        FlyCapture2::Image cimg;
        _Camera.RetrieveBuffer(&cimg);
        _Camera.RetrieveBuffer(&cimg);
        _TimeStamp = cimg.GetTimeStamp();
        cs = _TimeStamp.cycleSeconds;
    }
#endif
    ////////////////////////////////////////////////////

    //Load a reference image for contrast analyzation
    cv::Mat ref;
    if (_Calibration->doCalibration) {
        FILE *fp = fopen("refIm.jpg", "r");
        if (fp) {
            ref = cv::imread("refIm.jpg", CV_LOAD_IMAGE_GRAYSCALE);
            fclose(fp);
        } else {
            std::cout << "Warning: not found reference image refIm.jpg."
                      << std::endl;
        }
    }

    while (1) {
        _Dog->pulse(_ID);
        FlyCapture2::Image cimg;

        std::chrono::steady_clock::time_point end =
            std::chrono::steady_clock::now();
        //Retrieve image and metadata
        Error e = _Camera.RetrieveBuffer(&cimg);
	//Get the timestamp
	std::string currentTimestamp = get_utc_time();

        std::chrono::steady_clock::time_point begin =
            std::chrono::steady_clock::now();

        //Check if processing a frame took longer than 0.4 seconds. If so, log the event.
        int duration = std::chrono::duration_cast<std::chrono::microseconds>(
                           begin - end).count();
        if (duration > 333333) {
            std::cout << "Warning: Processing time too long:  " << duration
                      << "\n";
            QString str("Warning: Processing time too long: ");
            str.append(std::to_string(duration).c_str());
            str.append(" on camera ");
            str.append(std::to_string(_ID).c_str());
            generateLog(logfilepathFull, str);
        }

        //In case an error occurs, simply log it and restart the application.
        //Sometimes fetching images starts to fail and cameras need to be re-initialized.
        //However, this will solve the issue in the general case
        if (e.GetType() != PGRERROR_OK) {
            logCriticalError(e);
            std::exit(1);
        }

        //Grab metadata and timestamps from the images
        _ImInfo = cimg.GetMetadata();
        _FrameNumber = _ImInfo.embeddedFrameCounter;
        _TimeStamp = cimg.GetTimeStamp();

        //converts the time in seconds to local time
        timeinfo = localtime(&_TimeStamp.seconds);

        localCounter(oldTime, timeinfo->tm_sec);

        //////////////////////////////////////////////

        if (_TimeStamp.microSeconds > oldTimeUs) {
            difTimeStampUs = _TimeStamp.microSeconds - oldTimeUs;
        }
        else {
            difTimeStampUs = _TimeStamp.microSeconds - oldTimeUs + 1000000;
        }

        oldTimeUs = _TimeStamp.microSeconds;

        ///////////////////////////////////

        oldTime = timeinfo->tm_sec;

        //Prepare and put the image into the buffer
        //std::string currentTimestamp(timeresult);

        //Not in calibration mode. Move image to buffer for further procession
        if (!_Calibration->doCalibration) {
            std::shared_ptr<beeCompress::ImageBuffer> buf = std::shared_ptr<
                    beeCompress::ImageBuffer>(
                        new beeCompress::ImageBuffer(vwidth, vheight, _ID,
                                currentTimestamp));
            //int numBytesRead = flycapTo420(buf.get()->data, &cimg);
            memcpy(buf.get()->data, cimg.GetData(), vwidth * vheight);

#ifndef USE_ENCODER
            _Buffer->push(buf);
#endif
            _SharedMemBuffer->push(buf);

            //For every 50th picture create image statistics
            //if (loopCount % 50 == 0)
            //_AnalysisBuffer->push(buf);
        }
        //Calibrating cameras only
        else if (loopCount % 3 == 0) {

            //Analyze image properties
            analyzeImage(_ID, &cimg, &ref, _Calibration);
        }

        loopCount++;
    }
    _Camera.StopCapture();
    _Camera.Disconnect();

    std::cout << "done" << std::endl;
    return;
}

void Flea3CamThread::logCriticalError(Error e) {
    char logfilepathFull[256];
    std::stringstream str;
    SettingsIAC *set = SettingsIAC::getInstance();
    std::string logdir = set->getValueOfParam<std::string>(
                             IMACQUISITION::LOGDIR);
    str << "Error acquiring image. Printing full info and exiting. "
        << std::endl;
    str << "Type: "                 << e.GetType()              << std::endl;
    str << "Description: "          << e.GetDescription()       << std::endl;
    str << "Filename: "             << e.GetFilename()          << std::endl;
    str << "Line: "                 << e.GetLine()              << std::endl;
    str << "ErrorType: "            << e.GetType()              << std::endl;
    Error cause = e.GetCause();
    str << "Cause.Type: "           << cause.GetType()          << std::endl;
    str << "Cause.Description: "    << cause.GetDescription()   << std::endl;
    str << "Cause.Filename: "       << cause.GetFilename()      << std::endl;
    str << "Cause.Line: "           << cause.GetLine()          << std::endl;
    str << "Cause.ErrorType: "      << cause.GetType()          << std::endl;
    str << "Exit! "                                             << std::endl;
    sprintf(logfilepathFull, logdir.c_str(), _ID);
    generateLog(logfilepathFull, str.str().c_str());
}

// We will use Format7 to set the video parameters instead of DCAM, so it becomes handy to print this info
void Flea3CamThread::PrintFormat7Capabilities(Format7Info fmt7Info) {
    sendLogMessage(3,
                   "Max image pixels: " + QString::number(fmt7Info.maxWidth) + " x "
                   + QString::number(fmt7Info.maxHeight) + "\n"
                   + "Image Unit size: "
                   + QString::number(fmt7Info.imageHStepSize) + " x "
                   + QString::number(fmt7Info.imageVStepSize) + "\n"
                   + "Offset Unit size: "
                   + QString::number(fmt7Info.offsetHStepSize) + " x "
                   + QString::number(fmt7Info.offsetVStepSize) + "\n"
                   + "Pixel format bitfield: "
                   + QString::number(fmt7Info.pixelFormatBitField));
}

// Just prints the camera's info
void Flea3CamThread::PrintCameraInfo(CameraInfo *pCamInfo) {
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
}

bool Flea3CamThread::checkReturnCode(Error error) {
    if (error != PGRERROR_OK) {
        sendLogMessage(1,
                       "Cam " + QString::number(_ID) + " : " + error.GetDescription());
        return false;
    }
    return true;
}

void Flea3CamThread::sendLogMessage(int logLevel, QString message) {
    emit logMessage(logLevel, "Cam " + QString::number(_ID) + " : " + message);
}

void Flea3CamThread::cleanFolder(QString path, QString message) {
    QDir dir(path);
    dir.setNameFilters(QStringList() << "*.jpeg");
    dir.setFilter(QDir::Files);
    foreach(QString dirFile, dir.entryList()) {
        dir.remove(dirFile);
    }
    QString filename = (path + "log.txt");
    QFile file(filename);
    file.open(QIODevice::Append);
    QTextStream stream(&file);
    stream << message << "\r\n";
    file.close();
}

void Flea3CamThread::generateLog(QString path, QString message) {
    QString filename = (path + "log.txt");
    QFile file(filename);
    file.open(QIODevice::Append);
    QTextStream stream(&file);
    stream << QString(getTimestamp().c_str()) << ": " << message << "\r\n";
    file.close();
}

void Flea3CamThread::localCounter(unsigned int oldTime, unsigned int newTime) {
    if (oldTime != newTime) {
        _LocalCounter = 0;
    }
    _LocalCounter++;
}
