// SPDX-License-Identifier: GPL-3.0-or-later

#include "ImgAcquisitionApp.h"
#include "settings/Settings.h"
#include "settings/ParamNames.h"
#include "settings/utility.h"
#include "Watchdog.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <QDebug>
#include <QDir>
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

#ifdef __linux__
void cpsleep(int t)
{
    usleep(t);
}
#else
void cpsleep(int t)
{
    Sleep(t);
}
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
    std::string  imdirSet       = set->getValueOfParam<std::string>(IMACQUISITION::IMDIR);
    std::string  imdirprevSet   = set->getValueOfParam<std::string>(IMACQUISITION::IMDIRPREVIEW);
    std::string  exchangedirSet = set->getValueOfParam<std::string>(IMACQUISITION::EXCHANGEDIR);
    std::string  exchangedirprevSet = set->getValueOfParam<std::string>(
        IMACQUISITION::EXCHANGEDIRPREVIEW);

    std::size_t found       = imdirSet.find_last_of("/\\");
    std::string imdir       = imdirSet.substr(0, found) + "/";
    found                   = exchangedirSet.find_last_of("/\\");
    std::string exchangedir = exchangedirSet.substr(0, found) + "/";

    found                       = imdirprevSet.find_last_of("/\\");
    std::string imdirprev       = imdirprevSet.substr(0, found) + "/";
    found                       = exchangedirprevSet.find_last_of("/\\");
    std::string exchangedirprev = exchangedirprevSet.substr(0, found) + "/";

    for (int i = 0; i < 4; i++)
    {
        char src[512];
        char dst[512];
        sprintf(src, imdir.c_str(), i, 0);
        sprintf(dst, exchangedir.c_str(), i, 0);
        resolveLockDir(src, dst);
        sprintf(src, imdirprev.c_str(), i, 0);
        sprintf(dst, exchangedirprev.c_str(), i, 0);
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
    CalibrationInfo calib;
    Watchdog        dog;
    int             numCameras  = 0;
    int             camsStarted = 0;
    SettingsIAC*    set         = SettingsIAC::getInstance();
    calib.doCalibration         = false;

    // When calibrating cameras only
    // Do not do image analysis on regular recordings
    // analysis              = new beeCompress::ImageAnalysis(
    //      set->getValueOfParam<std::string>(IMACQUISITION::ANALYSISFILE), &dog);

    _smthread = new beeCompress::SharedMemory();

    std::cerr << "Successfully parsed config!" << std::endl;

    if (argc > 1 && strncmp(argv[1], "--help", 6) == 0)
    {
        std::cout << "Usage: ./bb_imageacquision <Options>" << std::endl
                  << "Valid options: " << std::endl
                  << "(none) \t\t start recording as per config." << std::endl
                  << "--calibrate \t Camera calibration mode." << std::endl;
        QCoreApplication::exit(0);
        std::exit(0);
    }
    else if (argc > 1 && strncmp(argv[1], "--calibrate", 11) == 0)
    {
        calib.doCalibration = true;
    }

    printBuildInfo();
    resolveLocks();

    // when the number of cameras is insufficient it should interrupt the program
    numCameras = checkCameras();

    int camcountConf = set->getValueOfParam<int>(IMACQUISITION::CAMCOUNT);
    if (numCameras < camcountConf)
    {
        std::cerr << "Camera count is less than configured!" << std::endl;
    }

    if (numCameras < 1)
    {
        std::exit(2);
    }

    // Initialize CamThreads and connect the respective signals.
    for (int i = 0; i < 4; i++)
    {

#ifdef USE_FLEA3
        _threads[i] = std::make_unique<Flea3CamThread>();
#endif

#ifdef USE_XIMEA
        _threads[i] = std::make_unique<XimeaCamThread>();
#endif

#ifdef USE_BASLER
        _threads[i] = std::make_unique<BaslerCamThread>();
#endif

        connect(_threads[i].get(),
                SIGNAL(logMessage(int, QString)),
                this,
                SLOT(logMessage(int, QString)));
    }

    std::cout << "Connected " << numCameras << " cameras." << std::endl;

    // the threads are initialized as a private variable of the class ImgAcquisitionApp
    _threads[0]->initialize(0, (_glue1._Buffer1), _smthread->_Buffer, &calib, &dog);
    _threads[1]->initialize(1, (_glue2._Buffer1), _smthread->_Buffer, &calib, &dog);
    _threads[2]->initialize(2, (_glue1._Buffer2), _smthread->_Buffer, &calib, &dog);
    _threads[3]->initialize(3, (_glue2._Buffer2), _smthread->_Buffer, &calib, &dog);

    // Map the buffers to camera id's
    _glue1._CamBuffer1 = 0;
    _glue1._CamBuffer2 = 2;
    _glue2._CamBuffer1 = 1;
    _glue2._CamBuffer2 = 3;

    std::cout << "Initialized " << numCameras << " cameras." << std::endl;

    // execute run() function, spawns cam readers
    for (int i = 0; i < 4; i++)
    {
        if (_threads[i]->isInitialized())
        {
            _threads[i]->start();
            camsStarted++;
        }
    }

    std::cout << "Started " << camsStarted << " camera threads." << std::endl;

    // Start encoder threads
    // The if(numCameras>=2) is not required here. If only one camera is present,
    // the other glue thread will sleep most of the time.
    _glue1.start();
    _glue2.start();

    std::cout << "Started the encoder threads." << std::endl;

    // While normal recording, start analysis thread to
    // log image statistics
    if (!calib.doCalibration)
    {
        _smthread->start();
        std::cout << "Started shared memory thread." << std::endl;
    }

    // Do output for calibration process
    while (calib.doCalibration)
    {
#ifdef __linux__
        system("clear");
#else
        system("cls");
#endif
        std::cout << "*************************************" << std::endl
                  << "*** Doing camera calibration only ***" << std::endl
                  << "*************************************" << std::endl;
        printf("CamId,\tSMD,\tVar,\tCont,\tNoise\n");
        calib.dataAccess.lock();
        for (int i = 0; i < 4; i++)
            printf("Cam %d: %f,\t%f,\t%f,\t%f\n",
                   i,
                   calib.calibrationData[i][0],
                   calib.calibrationData[i][1],
                   calib.calibrationData[i][2],
                   calib.calibrationData[i][3]);

        calib.dataAccess.unlock();
        cpsleep(500 * 1000);
    }

    cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);

    while (true)
    {
        dog.check();

#ifdef WITH_DEBUG_IMAGE_OUTPUT

        for (int i = 0; i < 5 * 1000; ++i)
            cv::waitKey(100);
#else
        cpsleep(500 * 1000);
#endif
    }
}

// destructor
ImgAcquisitionApp::~ImgAcquisitionApp()
{
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
