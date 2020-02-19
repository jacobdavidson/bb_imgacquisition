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

#if HALIDE
#include "halideYuv420Conv.h"
#include "Halide.h"
//On Windows this might conflict with predefs
#undef user_error
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
    

    return _initialized;
}

bool BaslerCamThread::initCamera()
{
    _initialized = false;

    
    return _initialized;
}

// This function starts the streaming from the camera
bool BaslerCamThread::startCapture() {
    
    return true;
}

void BaslerCamThread::run() {
    
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
