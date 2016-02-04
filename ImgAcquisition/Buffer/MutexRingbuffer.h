/*
 * MutexRingbuffer.h
 *
 *  Created on: Nov 10, 2015
 *      Author: hauke
 */

#ifndef MUTEXRINGBUFFER_H_
#define MUTEXRINGBUFFER_H_
#include <mutex>
#include "FlyCapture2.h"

namespace beeCompress {

/**
 * @brief The MutexRingbuffer class
 *
 * This class provides a ringbuffer for caching
 * recorded images. Reading and writing is only safe
 * for one thread each.
 */
class MutexRingbuffer{
	/**
	 * @brief _Images Array representing the image cache.
	 */
	FlyCapture2::Image** _Images;

	/**
	 * @brief _Frontp Pointer to the next image.
	 */
	int _Frontp;

	/**
	 * @brief _Backp Pointer to the element which will be returned next by 'back'.
	 */
	int _Backp;

	/**
	 * @brief _Size Size of the ringbuffer.
	 */
	int _Size;

	/**
	 * @brief _Access Mutex to modify the ringbuffer.
	 */
	std::mutex _Access;

public:

	/**
	 * @brief _Count The amount of images being cached.
	 */
	int _Count;

    /**
     * @brief Creates a new ringbuffer.
     *
     * All slots are pre-initialized with FlyCap image objects.
     *
     * @param Length of the ringbuffer
     */
	MutexRingbuffer(int pSize);
	~MutexRingbuffer();

    /**
     * @brief Returns pointer to the next image object.
     *
     * Using this, a handle to the next image to work on can be retrieved
     * for the writer thread. The reader will not be able to access this
     * object unless 'push' has been called.
     *
     * @return 	Pointer to the work image.
     */
	FlyCapture2::Image* front();

    /**
     * @brief Returns pointer to the next image to process
     *
     * Using this, a handle to the next image to process, i.e. encode can
     * be retrieved by the writer thread. This pops the element and thus
     * may only be only called once for each element.
     *
     * @return 	Pointer to the next image or NULL, if buffer is empty.
     */
	FlyCapture2::Image* back();

    /**
     * @brief Releases next slot in the buffer to be read.
     *
     * The image which was previously returned by 'front' may
     * now be accessed using 'back'.
     */
	void push();

};

} /* namespace beeCompress */

#endif /* MUTEXRINGBUFFER_H_ */
