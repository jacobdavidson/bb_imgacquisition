#include "BaslerCamThread.h"

#include <cstdlib>
#include <iostream>

#include <QDebug>
#include <qdir.h>       //QT stuff
#include <qtextstream.h>

#include "Watchdog.h"
#include "settings/Settings.h"
#include "settings/utility.h"
#include "ImageAnalysis.h"
#include <sstream> //stringstreams

#include <array>
#include <ctime> //get time
#include <time.h>

#if __linux__
#include <unistd.h> //sleep
#include <sys/time.h>
#else
#include <stdint.h>
#endif

#include <opencv2/opencv.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "boost/date_time/local_time/local_time.hpp"
#include "boost/date_time/c_local_time_adjustor.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/date_time.hpp>

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

BaslerCamThread::BaslerCamThread() {

}

BaslerCamThread::~BaslerCamThread() {

}

//this function reads the data input vector
bool BaslerCamThread::initialize(unsigned int id,
                                beeCompress::MutexBuffer *pBuffer,
                                beeCompress::MutexBuffer *pSharedMemBuffer, CalibrationInfo *calib,
                                Watchdog *dog) 
{
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
        if (_initialized)
            std::cout << "Done starting capture." << std::endl;
        else
            std::cout << "Error capturing." << std::endl;
    } 
    else {
        std::cerr << "Error on camera init " << id << std::endl;
        return false;
    }

    return _initialized;
}

bool BaslerCamThread::initCamera()
{
    _initialized = false;

    SettingsIAC *set            = SettingsIAC::getInstance();
    EncoderQualityConfig cfg    = set->getBufferConf(_ID, 0);

    // If the configuration entry is invalid, just skip it.
    if (cfg.serialString.empty())
    {
        sendLogMessage(0, "Serial number not configured. Skipping camera discovery.");
        return false;
    }

    try
    {
         // Get the transport layer factory.
        CTlFactory& tlFactory = CTlFactory::GetInstance();

        // Get all attached devices and return -1 if no device is found.
        DeviceInfoList_t devices;

        int n = tlFactory.EnumerateDevices(devices);        

        _HWID = std::numeric_limits<decltype(_HWID)>::max();
        for (int i = 0; i < n; i++)
        {
            if (  cfg.serialString == devices[i].GetSerialNumber().c_str()  )
            {
                _HWID = i;
                _camera.Attach( tlFactory.CreateDevice( devices[i] ));
                _initialized = true;

                return true;
            }
        }

        if (_HWID == std::numeric_limits<decltype(_HWID)>::max())
        {
            sendLogMessage(0, "failed to find cam matching config serial nr: " + cfg.serialString);
            return false;
        }
        
        // ToDo:
        // print additional info
        // set cam params as set in config file
    }
    catch(GenericException e)
    {

    }
    
    return _initialized;
}

// This function starts the streaming from the camera
bool BaslerCamThread::startCapture() {

    try{
        _camera.StartGrabbing();
    }
    catch(GenericException e)
    {
        return false;
    }
    
    return true;
}

void BaslerCamThread::run() 
{
    char logfilepathFull[256];

    SettingsIAC *set            = SettingsIAC::getInstance();
    EncoderQualityConfig cfg    = set->getBufferConf(_ID, 0);
    std::string logdir          = set->getValueOfParam<std::string>(IMACQUISITION::LOGDIR);

    const unsigned int vwidth   = static_cast<unsigned int>(cfg.width);
    const unsigned int vheight  = static_cast<unsigned int>(cfg.height);

    ////////////////////////WINDOWS/////////////////////
    int oldTime = 0;
    ////////////////////////////////////////////////////

    ////////////////////////LINUX/////////////////////
#ifdef __linux__
    //Timestamp housekeeping
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);
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
    
    // The camera timestamp will be used to get a more accurate idea of when the image was taken.
    // Software hangups (e.g. short CPU spikes) can thus be mitigated.
    unsigned long lastCameraTimestampMicroseconds   {0};
    unsigned long lastImageSequenceNumber           {0};
    boost::posix_time::ptime lastCameraTimestamp;

    // Preallocate image buffer on stack in order to safe performance later.
    std::array<unsigned char, 3008 * 4112> imageBuffer;

    for (size_t loopCount = 0; true; loopCount += 1)
    {
        _Dog->pulse(static_cast<int>(_ID));

        // CONTINUE HERE
        XI_IMG image;
        image.size = sizeof(XI_IMG);
        image.bp = static_cast<LPVOID> (&imageBuffer[0]);
        image.bp_size = imageBuffer.size();

        const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        //Retrieve image and metadata
        auto returnCode = xiGetImage(_Camera, 1000 * 2, &image);
        //Get the timestamp
        const auto wallClockNow = boost::posix_time::microsec_clock::universal_time();
        const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        const unsigned long currentCameraTimestampMicroseconds = image.tsUSec;

        // Image sequence sanity check.
        if (lastImageSequenceNumber != 0 && image.nframe != lastImageSequenceNumber + 1)
        {
            QString str("Warning: Camera lost frame: This frame: #");
            str.append(std::to_string(image.nframe).c_str());
            str.append(" last frame: #");
            str.append(std::to_string(lastImageSequenceNumber).c_str());
            str.append(" timestamp: ");
            str.append(boost::posix_time::to_iso_extended_string(wallClockNow).c_str());
            std::cout << str.toStdString() << std::endl;
            generateLog(logfilepathFull, str);

            for (auto &what : std::map<std::string, int> {{"Transport layer loss: ", XI_CNT_SEL_TRANSPORT_SKIPPED_FRAMES},
                                {"API layer loss: ", XI_CNT_SEL_API_SKIPPED_FRAMES}})
            {
                xiSetParamInt(_Camera, XI_PRM_COUNTER_SELECTOR, what.second);
                int result {0};
                xiGetParamInt(_Camera, XI_PRM_COUNTER_VALUE, &result);
                std::cout << what.first << result << std::endl;
            }

        }
        lastImageSequenceNumber = image.nframe;

        // Reset the camera timestamp?
        if (lastCameraTimestampMicroseconds == 0 || (lastCameraTimestampMicroseconds > currentCameraTimestampMicroseconds))
        {
            if (lastCameraTimestampMicroseconds != 0 && lastCameraTimestamp > wallClockNow && loopCount > 10)
            {
                QString str("Warning: camera clock faster than wall time. Last camera time: ");
                str.append(boost::posix_time::to_iso_extended_string(lastCameraTimestamp).c_str());
                str.append(" wall clock: ");
                str.append(boost::posix_time::to_iso_extended_string(wallClockNow).c_str());
                std::cout << str.toStdString() << std::endl;
                generateLog(logfilepathFull, str);
            }
            lastCameraTimestamp = wallClockNow;
        }
        else
        {
            const long microsecondDelta = static_cast<const long>(currentCameraTimestampMicroseconds - lastCameraTimestampMicroseconds);
            assert (microsecondDelta >= 0);
            lastCameraTimestamp += boost::posix_time::microseconds(microsecondDelta);
        }
        lastCameraTimestampMicroseconds = currentCameraTimestampMicroseconds;

        // Skip the first X images to allow the camera buffer to be flushed.
        if (loopCount < 10)
        {
            continue;
        }

        //Check if processing a frame took longer than X seconds. If so, log the event.
        const long duration = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
        if (duration > 2 * (1000000 / 6)) {
            QString str("Warning: Processing time too long: ");
            str.append(std::to_string(duration).c_str());
            str.append(" on camera ");
            str.append(std::to_string(_ID).c_str());
            std::cout << str.toStdString() << std::endl;
            generateLog(logfilepathFull, str);
        }

        //In case an error occurs, simply log it and restart the application.
        //Sometimes fetching images starts to fail and cameras need to be re-initialized.
        //However, this will solve the issue in the general case
        if (returnCode != XI_OK) {
            const std::string message = "Aquisition failed with error code " + std::to_string(returnCode);
            logCriticalError(message, message);
            std::exit(1);
        }


        //converts the time in seconds to local time
        const std::time_t timestamp { image.tsSec };
        const struct tm *timeinfo { localtime(&timestamp) };

        localCounter(oldTime, timeinfo->tm_sec);
        oldTime = timeinfo->tm_sec;

        //Not in calibration mode. Move image to buffer for further procession
        if (!_Calibration->doCalibration) {
            // Crop the image to the expected size (e.g. 4000x3000).
            // This is necessary, because the encoder/codec requires the image sizes to be some multiple of X.
            cv::Mat wholeImageMatrix(cv::Size(static_cast<int>(image.width), static_cast<int>(image.height)), CV_8UC1, image.bp, cv::Mat::AUTO_STEP);
            const unsigned int marginToBeCroppedX = (image.width > vwidth) ? image.width - vwidth : 0;
            const unsigned int marginToBeCroppedY = (image.height > vheight) ? image.height - vheight : 0;
            if (marginToBeCroppedX > 0 || marginToBeCroppedY > 0)
            {
                const int cropLeft = marginToBeCroppedX / 2;
                const int cropTop = marginToBeCroppedY / 2;
                cv::Mat croppedImageMatrix = wholeImageMatrix(cv::Rect(cropLeft, cropTop, static_cast<int>(vwidth), static_cast<int>(vheight)));
                croppedImageMatrix.copyTo(wholeImageMatrix);
            }
            const std::string frameTimestamp = boost::posix_time::to_iso_extended_string(lastCameraTimestamp) + "Z";
            auto buf = std::make_shared<beeCompress::ImageBuffer>(vwidth, vheight, _ID, frameTimestamp);
            memcpy(buf.get()->data, wholeImageMatrix.data, vwidth * vheight);

#ifndef USE_ENCODER
            _Buffer->push(buf);
#endif
            _SharedMemBuffer->push(buf);

#ifdef WITH_DEBUG_IMAGE_OUTPUT
            {
                cv::Mat smallMat;
                cv::resize(wholeImageMatrix, smallMat, cv::Size(400, 300));
                cv::imshow("Display window", smallMat );
            }
#endif
            //For every 50th picture create image statistics
            //if (loopCount % 50 == 0)
            //_AnalysisBuffer->push(buf);
        }
        //Calibrating cameras only
        else if (loopCount % 3 == 0) {

            //Analyze image properties
            assert(false);
            //analyzeImage(_ID, &cimg, &ref, _Calibration);
        }
    }
    // This code will never be executed.
    assert (false);
    auto errorCode = xiStopAcquisition(_Camera);
    checkReturnCode(errorCode, "xiStopAcquisition");
    errorCode = xiCloseDevice(_Camera);
    checkReturnCode(errorCode, "xiCloseDevice");

    std::cout << "done" << std::endl;    
    
    return;
}

void BaslerCamThread::logCriticalError(const std::string &shortMsg, const std::string &message) {
    
    return;
}

void BaslerCamThread::sendLogMessage(int logLevel, QString message) {
    emit logMessage(logLevel, "Cam " + QString::number(_ID) + " : " + message);
}
void BaslerCamThread::sendLogMessage(int logLevel, std::string message) {
    sendLogMessage(logLevel, QString::fromStdString(message));
}
void BaslerCamThread::sendLogMessage(int logLevel, const char *message) {
    sendLogMessage(logLevel, QString(message));
}

void BaslerCamThread::cleanFolder(QString path, QString message) {
    QDir dir(path);
    dir.setNameFilters(QStringList() << "*.jpeg");
    dir.setFilter(QDir::Files);
    for (const auto & dirFile : dir.entryList()) {
        dir.remove(dirFile);
    }
    QString filename = (path + "log.txt");
    QFile file(filename);
    file.open(QIODevice::Append);
    QTextStream stream(&file);
    stream << message << "\r\n";
    file.close();
}

void BaslerCamThread::generateLog(QString path, QString message) {
    QString filename = (path + "log.txt");
    QFile file(filename);
    file.open(QIODevice::Append);
    QTextStream stream(&file);
    stream << QString(getTimestamp().c_str()) << ": " << message << "\r\n";
    file.close();
}

void BaslerCamThread::localCounter(int oldTime, int newTime) {
    if (oldTime != newTime) {
        _LocalCounter = 0;
    }
    _LocalCounter++;
}
