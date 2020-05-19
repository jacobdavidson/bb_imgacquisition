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
#include "PlatformAdapter.h"
#include "log.h"
#include "version.h"
#include "build_timestamp.h"

ImgAcquisitionApp::~ImgAcquisitionApp()
{
    for (auto& thread : _cameraThreads)
    {
        thread->requestInterruption();
    }

    for (auto& thread : _cameraThreads)
    {
        thread->wait();
    }

    // Ensure empty image is added to end of each video stream to signal possible termination
    // within video file to video writers
    _cameraThreads.clear();

    for (auto& [name, thread] : _videoWriterThreads)
    {
        thread.requestInterruption();
    }

    for (auto& [name, thread] : _videoWriterThreads)
    {
        thread.wait();
    }
}

ImgAcquisitionApp::ImgAcquisitionApp(int& argc, char** argv)
: QCoreApplication(argc, argv)
{
    int          numCameras  = 0;
    int          camsStarted = 0;
    SettingsIAC* set         = SettingsIAC::getInstance();

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

        if (_videoWriterThreads.count(cfg.encoder.id))
        {
            _videoWriterThreads.at(cfg.encoder.id).add(videoStream);
        }
        else
        {
            logCritical("Encoder with id {} referenced, but not configured", cfg.encoder.id);
        }
    }

    for (auto& thread : _cameraThreads)
    {
        thread->start();
    }

    logInfo("Started {} camera threads", _cameraThreads.size());

    for (auto& [id, thread] : _videoWriterThreads)
    {
        thread.start();
    }

    logInfo("Started {} video writer threads", _videoWriterThreads.size());

    auto watchdogTimer = new QTimer(this);
    watchdogTimer->setInterval(500);
    watchdogTimer->setSingleShot(false);
    watchdogTimer->start();
    connect(watchdogTimer, &QTimer::timeout, this, [this]() { _watchdog.check(); });

    auto* adapter = new PlatformAdapter(this);
    QObject::connect(adapter, &PlatformAdapter::interruptReceived, this, QCoreApplication::quit);
    QObject::connect(adapter, &PlatformAdapter::terminateReceived, this, QCoreApplication::quit);
}

// Just prints the library's info
void ImgAcquisitionApp::printBuildInfo()
{
    logInfo("Application source version: {}", g_SOURCE_VERSION);
    logInfo("Application build timestamp: {}", g_BUILD_TIMESTAMP);
}
