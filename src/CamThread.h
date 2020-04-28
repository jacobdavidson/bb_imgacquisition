// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QThread>
#include <mutex>

// Forward declare not-required types.
class Watchdog;

class MutexBuffer;

/*!\brief Thread object which acquires images from a camera.
 *
 * Contains functions to initialize the cameras and run the acquistion.
 */
class CamThread : public QThread
{
    Q_OBJECT

protected:
    CamThread() = default;

public:
    virtual ~CamThread();

    /**
     * @brief Initialization of cameras and configuration
     *
     * @param Virtual ID of the camera (0 to 3)
     * @param Buffer shared with the encoder thread
     * @param Watchdog to notifiy each acquisition loop (when running)
     */
    virtual bool initialize(unsigned int id,
                            MutexBuffer* pBuffer,
                            Watchdog*    dog) = 0; // here goes the camera ID (from 0 to 3)
    virtual bool isInitialized() const     = 0;

signals:
    void logMessage(int logLevel, QString message);
};
