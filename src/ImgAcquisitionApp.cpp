// SPDX-License-Identifier: GPL-3.0-or-later

#include "ImgAcquisitionApp.h"

#include <iostream>
#include <fstream>
#include <cstdio>

#include <QDebug>
#include <QDir>
#include <QTimer>

#include "settings/Settings.h"
#include "settings/utility.h"
#include "Watchdog.h"

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

    for (auto& [id, name] : set->videoEncoders())
    {
        _videoWriterThreads.emplace(id, name);
    }

    for (const auto& cfg : set->videoStreams())
    {
        auto videoStream = VideoStream{cfg.id,
                                       {cfg.camera.width, cfg.camera.height},
                                       cfg.framesPerSecond,
                                       cfg.framesPerFile,
                                       cfg.encoder.options};
        _cameraThreads.emplace_back(CamThread::make(cfg.camera, videoStream, &_watchdog));

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
    std::cout << "Application build date: " << __DATE__ << ", " << __TIME__ << std::endl
              << std::endl;
}

// The slot for signals generated from the threads
void ImgAcquisitionApp::logMessage(int prio, QString message)
{
    qDebug() << message;
}
