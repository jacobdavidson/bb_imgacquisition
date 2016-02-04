/*
 * NvEncGlue.h
 *
 *  Created on: Nov 6, 2015
 *      Author: hauke
 */

#ifndef NVENCGLUE_H_
#define NVENCGLUE_H_

#include "FlyCapture2.h"
//#include "MutexRingbuffer.h"
#include "Buffer/MutexBuffer.h"
#include "Buffer/MutexLinkedList.h"
#include <QThread>

namespace beeCompress {

/**
 * @brief The NvEncGlue class
 *
 * This class reads from two ringbuffers
 * and processes the leass occupied one
 * using the NvEncoder in HEVC.
 * This class should be spawned as a QThread.<br>
 * <br>
 * Warning:<br>
 * Keep in mind:<br>
 * - 	The number of processors you spawn.
 * 		Most Nvidia GPU's are only able to spawn 2 HEVC GPU encoders.
 * 		One thread will use one encoder and crash if no GPU encoder is available.<br>
 * - 	The resolution. Encoding may throw errors madly if 4096x4096 is exceeded.
 * 		This is a Nvidia GPU encoder limitation.
 */
class NvEncGlue : public QThread{
	Q_OBJECT   //generates the MOC

public:

	/**
	 * @brief _Ring1 The first ringbuffer to encode
	 */
	MutexLinkedList *_Buffer1;

	/**
	 * @brief _CamRing1 Cam number associated with the first ringbuffer
	 */
	int _CamBuffer1;

	/**
	 * @brief _Ring2 The second ringbuffer to encode
	 */
	MutexLinkedList *_Buffer2;

	/**
	 * @brief _CamRing2 Cam number associated with the second ringbuffer
	 */
	int _CamBuffer2;

	/**
	 * @brief Creates a new encoder glue. Initializes ringbuffers.
	 */
	NvEncGlue();

	/**
	 * @brief Destroy the encoder glue
	*/
	virtual ~NvEncGlue();

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

} /* namespace beeCompress */

#endif /* NVENCGLUE_H_ */
