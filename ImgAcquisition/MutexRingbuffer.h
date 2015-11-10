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
class MutexRingbuffer{

public:
	FlyCapture2::Image** _Images;
	int _Frontp;
	int _Backp;
	int _Size;
	int _Count;
	std::mutex _Access;

	MutexRingbuffer();
	~MutexRingbuffer();

	FlyCapture2::Image* front();

	FlyCapture2::Image* back();

	void push();

};

} /* namespace beeCompress */

#endif /* MUTEXRINGBUFFER_H_ */
