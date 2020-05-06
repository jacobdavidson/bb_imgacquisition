// SPDX-License-Identifier: GPL-3.0-or-later

#include "ImgAcquisitionApp.h"
#include "settings/Settings.h"
#include "settings/utility.h"
#include "Watchdog.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <QDebug>
#include <QDir>
#include <QTimer>

#ifdef LINUX
    #include <unistd.h>
#endif
#ifdef WINDOWS
    #include <windows.h>
    #include <stdint.h>
#endif

#ifdef USE_FLEA3
    #include "Flea3CamThread.h"
using namespace FlyCapture2;
#endif

#ifdef USE_XIMEA
    #include "XimeaCamThread.h"
#endif

#ifdef USE_BASLER
    #include "BaslerCamThread.h"
    #include <pylon/PylonIncludes.h>
using namespace Pylon;
#endif

std::string ImgAcquisitionApp::figureBasename(std::string infile)
{
    std::ifstream f(infile.c_str());
    std::string   line;
    std::string   fst = "", last = "";
    int           prevSize  = -1;
    int           lineCount = 0;

    // Simple check, file not found?
    if (!f)
    {
        return "-1";
    }

    while (std::getline(f, line))
    {
        if (fst.size() == 0)
        {
            fst = line;
        }
        if (static_cast<int>(last.size()) == prevSize || prevSize == -1)
        {
            last = line;
        }
        lineCount++;
    }

    if (lineCount < 3)
    {
        std::cerr << "<Restoring partial video> File can not be rename: Textfile has "
                     "insufficient info: "
                  << infile << std::endl;
        return "-1";
    }
    return fst + "--" + last.substr(6);
}

void ImgAcquisitionApp::resolveLockDir(std::string from, std::string to)
{
    // list for all files found in the directory
    QList<QString> files;

    QString qsFrom(from.c_str());
    QDir    directory(qsFrom);

    QFileInfoList infoList = directory.entryInfoList(QDir::NoDotAndDotDot | QDir::Files,
                                                     QDir::Name);
    for (int i = 0; i < infoList.size(); ++i)
    {
        QFileInfo curFile = infoList.at(i);

        if (curFile.isFile() && curFile.fileName().contains(".lck"))
        {
            std::string basename =
                curFile.fileName().left(curFile.fileName().length() - 3).toStdString();
            std::string lockfile      = from + basename + "lck";
            std::string srcvideofile  = from + basename + "avi";
            std::string srcframesfile = from + basename + "txt";
            std::string dstBasename   = figureBasename(srcframesfile);
            std::string dstvideofile  = to + dstBasename + ".avi";
            std::string dstframesfile = to + dstBasename + ".txt";

            // Some error happened figuring the filename
            if (dstBasename == "-1")
            {
                continue;
            }

            rename(srcvideofile.c_str(), dstvideofile.c_str());
            rename(srcframesfile.c_str(), dstframesfile.c_str());

            // Remove the lockfile, so others will be allowed to grab the video
            remove(lockfile.c_str());
        }
    }
}

void ImgAcquisitionApp::resolveLocks()
{
    SettingsIAC* set            = SettingsIAC::getInstance();
    std::string  imdirSet       = set->tmpPath();
    std::string  exchangedirSet = set->outDirectory();

    std::size_t found       = imdirSet.find_last_of("/\\");
    std::string imdir       = imdirSet.substr(0, found) + "/";
    found                   = exchangedirSet.find_last_of("/\\");
    std::string exchangedir = exchangedirSet.substr(0, found) + "/";

    for (int i = 0; i < 4; i++)
    {
        char src[512];
        char dst[512];
        sprintf(src, imdir.c_str(), i, 0);
        sprintf(dst, exchangedir.c_str(), i, 0);
        resolveLockDir(src, dst);
    }
}

// constructor
// definition of ImgAcquisitionApp, it configures the threads, connects them including the
// signals(input) to be read and the slots (output) the whole process is executed when the object
// is initialized in main.
ImgAcquisitionApp::ImgAcquisitionApp(int& argc, char** argv)
: QCoreApplication(argc, argv) //
{
    int          numCameras  = 0;
    int          camsStarted = 0;
    SettingsIAC* set         = SettingsIAC::getInstance();

    std::cerr << "Successfully parsed config!" << std::endl;

    if (argc > 1 && strncmp(argv[1], "--help", 6) == 0)
    {
        std::cout << "Usage: ./bb_imageacquision <Options>" << std::endl
                  << "Valid options: " << std::endl
                  << "(none) \t\t start recording as per config." << std::endl;
        QCoreApplication::exit(0);
        std::exit(0);
    }

    printBuildInfo();
    resolveLocks();

    // when the number of cameras is insufficient it should interrupt the program
    numCameras = checkCameras();

    if (numCameras < 1)
    {
        std::exit(2);
    }

    for (auto& [id, name] : set->videoEncoders())
    {
        _videoWriterThreads.emplace(id, name);
    }

    for (unsigned int i = 0; i < set->videoStreams().size(); i++)
    {
        const auto& cfg = set->videoStreams()[i];

        auto videoStream = VideoStream{cfg.id,
                                       {cfg.camera.width, cfg.camera.height},
                                       cfg.framesPerSecond,
                                       cfg.framesPerFile,
                                       cfg.encoder.options};

#ifdef USE_FLEA3
        _cameraThreads.emplace_back(std::make_unique<Flea3CamThread>(cfg.camera, videoStream, &_watchdog));
#endif

#ifdef USE_XIMEA
        _cameraThreads.emplace_back(std::make_unique<XimeaCamThread>(cfg.camera, videoStream, &_watchdog));
#endif

#ifdef USE_BASLER
        _cameraThreads.emplace_back(std::make_unique<BaslerCamThread>(cfg.camera, videoStream, &_watchdog));
#endif

        connect(_cameraThreads.back().get(),
                &CamThread::logMessage,
                this,
                &ImgAcquisitionApp::logMessage);

        if (_videoWriterThreads.count(cfg.encoder.id))
        {
            _videoWriterThreads.at(cfg.encoder.id).add(videoStream);
        }
        else
        {
            std::cerr << "No such encoder configured: " << cfg.encoder.id << std::endl;
        }
    }

    for (auto& thread : _cameraThreads)
    {
        thread->start();
    }

    std::cout << "Started " << _cameraThreads.size() << " camera threads." << std::endl;

    for (auto& [id, thread] : _videoWriterThreads)
    {
        thread.start();
    }

    std::cout << "Started " << _videoWriterThreads.size() << " video writer threads." << std::endl;

    auto watchdogTimer = new QTimer(this);
    watchdogTimer->setInterval(500);
    watchdogTimer->setSingleShot(false);
    watchdogTimer->start();
    connect(watchdogTimer, &QTimer::timeout, this, [this]() { _watchdog.check(); });
}

// Just prints the library's info
void ImgAcquisitionApp::printBuildInfo()
{
#ifdef USE_FLEA3
    FC2Version fc2Version;
    Utilities::GetLibraryVersion(&fc2Version);

    std::cout << "FlyCapture2 library version: " << fc2Version.major << "." << fc2Version.minor
              << "." << fc2Version.type << "." << fc2Version.build << std::endl
              << std::endl;
#endif

#ifdef USE_BASLER

#endif

    std::cout << "Application build date: " << __DATE__ << ", " << __TIME__ << std::endl
              << std::endl;
}

// This function checks that at least one camera is connected
// returns -1 if no camera is found
int ImgAcquisitionApp::checkCameras()
{
#ifdef USE_FLEA3
    FlyCapture2::BusManager cc_busMgr;
    FlyCapture2::Error      error;

    error = cc_busMgr.GetNumOfCameras(&_numCameras);
    if (error != PGRERROR_OK)
    {
        return -1;
    }
#endif

#ifdef USE_XIMEA
    {
        const auto errorCode = xiGetNumberDevices(&_numCameras);
        if (errorCode != XI_OK)
            return -1;
    }
#endif

#ifdef USE_BASLER
    PylonAutoInitTerm pylon;
    try
    {
        // Get the transport layer factory.
        CTlFactory& tlFactory = CTlFactory::GetInstance();

        // Get all attached devices and return -1 if no device is found.
        DeviceInfoList_t devices;
        if (tlFactory.EnumerateDevices(devices) == 0)
            return -1;

        _numCameras = devices.size();

        // ToDo: make max number of cameras accessible
        CInstantCameraArray cameras(std::min<size_t>(_numCameras, 4));

        // Create and attach all Pylon Devices.
        for (size_t i = 0; i < cameras.GetSize(); ++i)
        {
            cameras[i].Attach(tlFactory.CreateDevice(devices[i]));

            // Print the model name and serial number of the camera(s).
            std::cout << "Cam #" << i << ": " << cameras[i].GetDeviceInfo().GetModelName()
                      << ", SN:" << cameras[i].GetDeviceInfo().GetSerialNumber() << std::endl;
        }

        // Detach cameras in array
        cameras.DestroyDevice();
    }
    catch (const GenericException& e)
    {
        return -1;
    }
#endif

    qDebug() << "Number of cameras detected: " << _numCameras;

    // print cam serial numbers
    for (int i = 0; i < 4; i++)
    {

#ifdef USE_FLEA3
        unsigned int serial;
        Error        error = cc_busMgr.GetCameraSerialNumberFromIndex(i, &serial);
        if (error == PGRERROR_OK)
        {
            qDebug() << "Detected cam with serial: " << serial;
        }
#endif

#ifdef USE_XIMEA
        XI_RETURN errorCode;
        HANDLE    cam;
        errorCode = xiOpenDevice(i, &cam);
        if (errorCode != XI_OK)
        {
            qDebug() << QString("Error opening cam ") << i << ": error code " << errorCode;
            continue;
        }
        char serial[20] = "";
        errorCode       = xiGetParamString(cam, XI_PRM_DEVICE_SN, serial, sizeof(serial));
        if (errorCode == XI_OK)
        {
            qDebug() << "Detected cam with serial: " << serial;
        }
        xiCloseDevice(cam);
#endif
    }

    if (_numCameras < 1)
    {
        std::cerr << "Insufficient number of cameras... press Enter to exit." << std::endl;
        logMessage(1, "Insufficient number of cameras ...");
        getchar();
        return -1;
    }
    return _numCameras;
}

// The slot for signals generated from the threads
void ImgAcquisitionApp::logMessage(int prio, QString message)
{
    qDebug() << message;
}
