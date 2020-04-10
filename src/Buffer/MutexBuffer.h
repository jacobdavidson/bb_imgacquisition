// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * MutexBuffer.h
 *
 *  Created on: Jan 25, 2016
 *      Author: hauke
 */

#ifndef MUTEXBUFFER_H_
#define MUTEXBUFFER_H_

#include <mutex>
#include <string>
#include <memory>
#include "Semaphore.h"
#include "ImageBuffer.h"

// Hard limit in MiB a buffer shall never exceed
#define BUFFER_HARDLIMIT 5000

namespace beeCompress
{



    /* LIFO Queue */
    class MutexBuffer
    {
    public:
        Semaphore waiting;

        virtual void push(std::shared_ptr<ImageBuffer> imbuffer) = 0;

        virtual std::shared_ptr<beeCompress::ImageBuffer> pop() = 0;

        virtual int size() = 0;

        MutexBuffer(){};

        virtual ~MutexBuffer(){};
    };

} /* namespace beeCompress */

#endif /* MUTEXBUFFER_H_ */
