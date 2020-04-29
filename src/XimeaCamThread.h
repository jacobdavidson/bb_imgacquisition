// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef WINDOWS
    #include <windows.h>
#endif
#include "CamThread.h"
#include <xiApi.h>

#include "Watchdog.h"
#include <mutex>
#include <string>

/*!\brief Thread object which acquires images from a camera.
 *
 * Contains functions to initialize the cameras and run the acquistion.
 */
class XimeaCamThread : public CamThread
{
    Q_OBJECT

public:
    XimeaCamThread();          // constructor
    virtual ~XimeaCamThread(); // destructor

    /**
     * @brief Initialization of cameras and configuration
     *
     * @param Virtual ID of the camera (0 to 3)
     * @param Buffer shared with the video writer thread
     * @param Watchdog to notifiy each acquisition loop (when running)
     */
    virtual bool initialize(unsigned int                                   id,
                            ConcurrentQueue<std::shared_ptr<ImageBuffer>>* pBuffer,
                            Watchdog* dog) override; // here goes the camera ID (from 0 to 3)

    //! Object has been initialized using "initialize"
    virtual bool isInitialized() const override
    {
        return _initialized;
    }
    bool _initialized{false};

    //! Virtual ID of the camera
    unsigned int _ID;

    //! Hardware ID (id in bus order) of the camera. (used internally)
    unsigned int _HWID;

    //! Serial number of the camera
    std::string _Serial;

    //! Watchdog to notifiy each acquisition loop (set by initialize)
    Watchdog* _Dog;

private:
    bool initCamera();
    bool startCapture();

    //! @brief Just prints the camera's info
    // void                PrintCameraInfo(CameraInfo *pCamInfo);

    /**
     * @brief Checks whether the error code is OK and logs otherwise
     *
     * @param Ximea error code
     */
    bool checkReturnCode(XI_RETURN errorCode, const std::string& operation = "");

    /**
     * @brief Sends an error message.
     *
     * Currently all of them get logged to console.
     *
     * @param The log level (currently ugnored)
     * @param Message forwarded to QDebug()
     */
    void sendLogMessage(int logLevel, QString message);
    void sendLogMessage(int logLevel, std::string message);
    void sendLogMessage(int logLevel, const char* message);

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
    void localCounter(int oldTime, int newTime);

    /**
     * @brief Logs a critical error in very detail
     *
     * Logs are written to the logfile specified as per
     * JSON configuration.
     *
     * @param The error object
     */
    // void logCriticalError(Error e);
    void logCriticalError(const std::string& shortMsg, const std::string& message);

    //! Deprecated JPeg compression parameter
    // JPEGOption            _jpegConf;

    //! Handle returned by xiOpenDevice.
    HANDLE _Camera;

    //! ... to enumerate each image in a second
    unsigned int _LocalCounter;

    //! Buffer shared with the video writer thread
    ConcurrentQueue<std::shared_ptr<ImageBuffer>>* _Buffer;

protected:
    void run(); // this is the function that will be iterated indefinitely
};
