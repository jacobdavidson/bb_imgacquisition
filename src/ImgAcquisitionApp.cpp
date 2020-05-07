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
    std::cout << "Application build date: " << __DATE__ << ", " << __TIME__ << std::endl;
}

// The slot for signals generated from the threads
void ImgAcquisitionApp::logMessage(int prio, QString message)
{
    qDebug() << message;
}
