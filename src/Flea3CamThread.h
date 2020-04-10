// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FLEA3CAMTHREAD_H
#define FLEA3CAMTHREAD_H

#ifdef WINDOWS
    #include <windows.h>
#endif
#include "CamThread.h"
#include "FlyCapture2.h"
#include "Buffer/MutexBuffer.h"
#include "Watchdog.h"
#include <mutex>
using namespace FlyCapture2;

/*!\brief Thread object which acquires images from a camera.
 *
 * Inherits from Qthread for threading.
 * Contains functions to initialize the cameras and run the acquistion.
 */
class Flea3CamThread : public CamThread
{
    Q_OBJECT

public:
    Flea3CamThread();  // constructor
    ~Flea3CamThread(); // destructor

    /**
     * @brief Initialization of cameras and configuration
     *
     * @param Virtual ID of the camera (0 to 3)
     * @param Buffer shared with the encoder thread
     * @param Buffer shared with the shared memory thread
     * @param Pointer to calibration data storeage
     * @param Watchdog to notifiy each acquisition loop (when running)
     */
    virtual bool initialize(unsigned int              id,
                            beeCompress::MutexBuffer* pBuffer,
                            beeCompress::MutexBuffer* pSharedMemBuffer,
                            CalibrationInfo*          calib,
                            Watchdog* dog) override; // here goes the camera ID (from 0 to 3)

    //! Object has been initialized using "initialize"
    virtual bool isInitialized() const override
    {
        return _initialized;
    }
    bool _initialized;

    //! Virtual ID of the camera
    unsigned int _ID;

    //! Hardware ID (id in bus order) of the camera. (used internally)
    unsigned int _HWID;

    //! Serial number of the camera
    unsigned int _Serial;

    //! Pointer to calibration data storeage (set by initialize)
    CalibrationInfo* _Calibration;

    //! Watchdog to notifiy each acquisition loop (set by initialize)
    Watchdog* _Dog;

private:
    bool initCamera();
    bool startCapture();

    //! @brief Just prints the camera's info
    void PrintCameraInfo(CameraInfo* pCamInfo);

    //! @brief Prints the camera's format capabilities
    void PrintFormat7Capabilities(
        Format7Info fmt7Info); // We will use Format7 to set the video parameters instead of DCAM,
                               // so it becomes handy to print this info

    /**
     * @brief Checks whether the error code is OK and logs otherwise
     *
     * @param The error object
     */
    bool checkReturnCode(Error error);

    /**
     * @brief Sends an error message.
     *
     * Currently all of them get logged to console.
     *
     * @param The log level (currently ugnored)
     * @param Message forwarded to QDebug()
     */
    void sendLogMessage(int logLevel, QString message);

    /**
     * @brief Generates a log message to log.txt in the given path.
     *
     * @param Path to the log.txt file
     * @param Message to emit
     */
    void generateLog(QString path, QString message);

    /**
     * @brief Deprecated
     *
     * @param Deprecated
     * @param Deprecated
     */
    void localCounter(unsigned int oldTime, unsigned int newTime);

    /**
     * @brief Logs a critical error in very detail
     *
     * Logs are written to the logfile specified as per
     * JSON configuration.
     *
     * @param The error object
     */
    void logCriticalError(Error e);

    //! Deprecated JPeg compression parameter
    // JPEGOption            _jpegConf;

    //! objects that points to the camera
    Camera _Camera;

    //! object to catch the image temporally
    Image _Image;

    //! object needed to read the frame counter
    ImageMetadata _ImInfo;

    //! the current frame in Image
    unsigned int _FrameNumber;

    //! time stamp from the current frame
    TimeStamp _TimeStamp;

    //! ... to enumerate each image in a second
    unsigned int _LocalCounter;

    //! Buffer shared with the encoder thread
    beeCompress::MutexBuffer* _Buffer;

    //! Buffer shared with the shared memory thread
    beeCompress::MutexBuffer* _SharedMemBuffer;

protected:
    void run(); // this is the function that will be iterated indefinitely
};

#endif // FLEA3CAMTHREAD_H
