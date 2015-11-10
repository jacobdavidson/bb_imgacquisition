/*
 * NvEncGlue.h
 *
 *  Created on: Nov 6, 2015
 *      Author: hauke
 */

#ifndef NVENCGLUE_H_
#define NVENCGLUE_H_

#include "FlyCapture2.h"
#include "MutexRingbuffer.h"
#include <QThread>

namespace beeCompress {

class NvEncGlue : public QThread{
	Q_OBJECT   //generates the MOC

public:

	MutexRingbuffer _Ring1;
	MutexRingbuffer _Ring2;

	NvEncGlue();
	virtual ~NvEncGlue();

	std::string getTimestamp();

protected:
	void run(); //this is the function that will be iterated indefinitely
};

} /* namespace beeCompress */

#endif /* NVENCGLUE_H_ */
