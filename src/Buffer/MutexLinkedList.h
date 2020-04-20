// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * MutexLinkedList.h
 *
 *  Created on: Jan 25, 2016
 *      Author: hauke
 */

#ifndef MUTEXLINKEDLIST_H_
#define MUTEXLINKEDLIST_H_

#include "MutexBuffer.h"
#include <list>
#include <memory>

class MutexLinkedList : public MutexBuffer
{

public:
    std::list<std::shared_ptr<ImageBuffer>> images;

    /**
     * @brief _Access Mutex to modify the ringbuffer.
     */
    std::mutex _Access;

    virtual void push(std::shared_ptr<ImageBuffer> imbuffer);

    virtual std::shared_ptr<ImageBuffer> pop();

    // Simple function to get the current size of the buffer in elements.
    // Locks the data structure.
    virtual int size()
    {
        int tsize = 0;
        _Access.lock();
        tsize = images.size();
        _Access.unlock();
        return tsize;
    }

    MutexLinkedList();

    virtual ~MutexLinkedList();
};

#endif /* MUTEXLINKEDLIST_H_ */
