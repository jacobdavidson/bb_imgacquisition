// SPDX-License-Identifier: GPL-3.0-or-later

#include "ImgAcquisitionApp.hpp"

#include <iostream>

#include <QTimer>

#include "Settings.hpp"
#include "Watchdog.hpp"
#include "util/log.hpp"
#include "util/PlatformAdapter.hpp"

#include "source_version.hpp"
#include "build_timestamp.hpp"

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

    // Ensure empty image is added to end of each image stream to signal possible termination
    // within video file to video writers
    _cameraThreads.clear();

    for (auto& [name, thread] : _imageStreamsWriters)
    {
        thread.requestInterruption();
    }

    for (auto& [name, thread] : _imageStreamsWriters)
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

    logInfo("Source version: {}", g_SOURCE_VERSION);
    logInfo("Build timestamp: {}", g_BUILD_TIMESTAMP);
    logInfo("Start timestamp: {:e}", std::chrono::system_clock::now());

    qRegisterMetaType<GrayscaleImage>("GrayscaleImage");

    for (auto& [id, name] : settings.videoEncoders())
    {
        _imageStreamsWriters.emplace(id, name);
    }

    for (const auto& cfg : settings.imageStreams())
    {
        auto imageStream = ImageStream{cfg.id,
                                       {cfg.camera.params.width, cfg.camera.params.height},
                                       cfg.framesPerSecond,
                                       cfg.framesPerFile,
                                       cfg.encoder.options};
        _cameraThreads.emplace_back(Camera::make(cfg.camera, imageStream));
        const auto watchIndex = _watchdog.watch(cfg.id);
        QObject::connect(
            _cameraThreads.back().get(),
            &Camera::imageCaptured,
            [this, watchIndex](GrayscaleImage image) { _watchdog.pulse(watchIndex); });

        if (_imageStreamsWriters.count(cfg.encoder.id))
        {
            _imageStreamsWriters.at(cfg.encoder.id).add(imageStream);
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

    logDebug("Started {} camera threads", _cameraThreads.size());

    for (auto& [id, thread] : _imageStreamsWriters)
    {
        thread.start();
    }

    logDebug("Started {} image streams writer threads", _imageStreamsWriters.size());

    auto watchdogTimer = new QTimer(this);
    watchdogTimer->setInterval(500);
    watchdogTimer->setSingleShot(false);
    watchdogTimer->start();
    connect(watchdogTimer, &QTimer::timeout, this, [this]() { _watchdog.check(); });

    auto* adapter = new PlatformAdapter(this);
    QObject::connect(adapter, &PlatformAdapter::interruptReceived, this, &ImgAcquisitionApp::quit);
    QObject::connect(adapter, &PlatformAdapter::terminateReceived, this, &ImgAcquisitionApp::quit);
}

void ImgAcquisitionApp::quit()
{
    std::cerr << std::endl;
    QCoreApplication::quit();
}
