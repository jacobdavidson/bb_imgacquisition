// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef WINDOWS
    #include <windows.h>
#endif

#include <mutex>
#include <string>
#include <optional>

#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCamera.h>

#include "CamThread.h"
#include "Watchdog.h"

/*!\brief Thread object which acquires images from a camera.
 *
 * Contains functions to initialize the cameras and run the acquistion.
 */
class BaslerCamThread : public CamThread
{
    Q_OBJECT

private:
    Pylon::PylonAutoInitTerm pylon;

public:
    BaslerCamThread();          // constructor
    virtual ~BaslerCamThread(); // destructor

    /**
     * @brief Initialization of cameras and configuration
     *
     * @param Virtual ID of the camera (0 to 3)
     * @param Buffer shared with the video writer thread
     * @param Watchdog to notifiy each acquisition loop (when running)
     */
    virtual bool initialize(unsigned int                                      id,
                            ConcurrentQueue<std::shared_ptr<GrayscaleImage>>* pBuffer,
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
    std::optional<unsigned int> _HWID;
    Pylon::String_t             _SerialNumber;

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
    bool checkReturnCode(int errorCode, const std::string& operation = "");

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
     * @param The error message
     */
    void logCriticalError(const std::string& message);

    // Pylon camera object
    Pylon::CBaslerUsbInstantCamera _camera;
    // Pylon camera capture result
    Pylon::CGrabResultPtr _grabbed;

    //! ... to enumerate each image in a second
    unsigned int _LocalCounter;

    //! Buffer shared with the video writer thread
    ConcurrentQueue<std::shared_ptr<GrayscaleImage>>* _Buffer;

protected:
    void run(); // this is the function that will be iterated indefinitely
};
