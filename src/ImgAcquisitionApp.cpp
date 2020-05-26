// SPDX-License-Identifier: GPL-3.0-or-later

#include "ImgAcquisitionApp.h"

#include <iostream>

#include <QTimer>

#include "Settings.h"
#include "Watchdog.h"
#include "PlatformAdapter.h"
#include "util/log.h"

#include "source_version.h"
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
    auto& settings = Settings::instance();

    const auto args = arguments();

    if (args.size() > 1 && args.at(1) == "--help")
    {
        std::cout << "Usage: ./bb_ImageAcquistion" << std::endl
                  << "Start recording as per settings file." << std::endl;
        QCoreApplication::exit(0);
        std::exit(0);
    }

    logInfo("Application: Start timestamp: {:e}", std::chrono::system_clock::now());
    logInfo("Application: Source version: {}", g_SOURCE_VERSION);
    logInfo("Application: Build timestamp: {}", g_BUILD_TIMESTAMP);

    qRegisterMetaType<GrayscaleImage>("GrayscaleImage");

    for (auto& [id, name] : settings.videoEncoders())
    {
        _videoWriterThreads.emplace(id, name);
    }

    for (const auto& cfg : settings.videoStreams())
    {
        auto videoStream = VideoStream{cfg.id,
                                       {cfg.camera.width, cfg.camera.height},
                                       cfg.framesPerSecond,
                                       cfg.framesPerFile,
                                       cfg.encoder.options};
        _cameraThreads.emplace_back(Camera::make(cfg.camera, videoStream, &_watchdog));

        if (_videoWriterThreads.count(cfg.encoder.id))
        {
            _videoWriterThreads.at(cfg.encoder.id).add(videoStream);
        }
        else
        {
            logCritical("Application: Encoder with id {} referenced, but not configured",
                        cfg.encoder.id);
        }
    }

    for (auto& thread : _cameraThreads)
    {
        thread->start();
    }

    logDebug("Application: Started {} camera threads", _cameraThreads.size());

    for (auto& [id, thread] : _videoWriterThreads)
    {
        thread.start();
    }

    logDebug("Application: Started {} video writer threads", _videoWriterThreads.size());

    auto watchdogTimer = new QTimer(this);
    watchdogTimer->setInterval(500);
    watchdogTimer->setSingleShot(false);
    watchdogTimer->start();
    connect(watchdogTimer, &QTimer::timeout, this, [this]() { _watchdog.check(); });

    auto* adapter = new PlatformAdapter(this);
    QObject::connect(adapter, &PlatformAdapter::interruptReceived, this, QCoreApplication::quit);
    QObject::connect(adapter, &PlatformAdapter::terminateReceived, this, QCoreApplication::quit);
}
