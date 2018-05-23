/*
 * SharedMemory.h
 *
 *  Created on: Jun 8, 2016
 *      Author: hauke
 */

#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_
#include <QThread>
#include "Buffer/MutexBuffer.h"
#include "Buffer/MutexLinkedList.h"
#include <boost/interprocess/sync/interprocess_mutex.hpp>

namespace beeCompress {

class SharedMemory : public QThread
{
    Q_OBJECT   //generates the MOC
public:
    /**
     * @brief Simple constuctor. Only creates the buffer.
     */
    SharedMemory();

    /**
     * @brief STUB
     */
    virtual ~SharedMemory();

    /**
     * @brief Creates a shared memory segment
     *
     * All but the last parameter are output parameters.
     * Memory layout:
     * - image data. Size: WidthxHeight
     * - Boost interprocess mutex object
     *
     * @param (out) Shared memory key
     * @param (out) Shared memory id
     * @param (out) Pointer to the memory segment
     * @param Id of the camera
     */
    boost::interprocess::interprocess_mutex *createSharedMemory(key_t *key, int *shmid, char **data, int id);

    //! Buffer which to feed the shared memory from
    MutexLinkedList *_Buffer;

    /**
     * @brief lock the shared memory mutex.
     *
     * Uses try_lock() to lock. If it fails enough many times in a second,
     * the lock is deleted and a new one is being created. This is to
     * prevent starvation.
     */
    void doLock(int id);

    /**
     * @brief unlock the shared memory mutex.
     *
     * Wraps lock() just to keep function calls similar.
     */
    void doUnlock(int id);
protected:

    ////////////////////////Shared Memory///////////////#

    //! Shared memory keys of camera 0 - 3
    key_t _key[4];

    //! Shared memory ids of camera 0 - 3
    int _shmid[4];

    //! Pointer to the memory segments of camera 0 - 3
    char *_data[4];

    //! Interprocess mutexes of camera 0 - 3
    boost::interprocess::interprocess_mutex *_mutex[4] = {nullptr, nullptr, nullptr, nullptr};
    ////////////////////////////////////////////////////

    /**
     * @brief Runs thread to supply shared memory indefinately
     */
    void run();
};

} /* namespace beeCompress */

#endif /* SHAREDMEMORY_H_ */
