// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Buffer/MutexBuffer.h"
#include "Buffer/MutexLinkedList.h"

#include <QThread>

/**
 * @brief The VideoWriteThread class
 *
 * This class reads from two ringbuffers
 * and processes the least occupied one
 * using the selected video encoder.
 *
 * Warning:
 * Keep in mind when using Nvidia hardware encoders:
 * -    The number of instances you spawn:
 *      Most consumer Nvidia GPUs are only able to spawn a limited amount of concurrent encoders, e.g. 2 for HEVC.
 *      One thread will use one encoder and crash if no GPU encoder is available.
 * -    The resolution:
 *      Encoding may not work correctly if 4096x4096 is exceeded.
 */
class VideoWriteThread final : public QThread
{
    Q_OBJECT

public:
    /**
     * @brief _Buffer1 The first buffer to encode
     */
    MutexLinkedList* _Buffer1;

    /**
     * @brief _CamRing1 Cam number associated with the first ringbuffer
     */
    int _CamBuffer1;

    /**
     * @brief _Buffer2 The second buffer to encode
     */
    MutexLinkedList* _Buffer2;

    /**
     * @brief _CamRing2 Cam number associated with the second ringbuffer
     */
    int _CamBuffer2;

    /**
     * @brief Creates a new encoder glue. Initializes ringbuffers.
     */
    VideoWriteThread();

protected:
    /**
     * @brief Run the encoding thread.
     *
     * This is the function that will be iterated indefinitely.
     * The ringbuffers will be watched permanently and will be encoded.
     *
     * @param Length of the ringbuffer
     */
    void run();
};
