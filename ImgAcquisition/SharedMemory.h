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
	SharedMemory();
	virtual ~SharedMemory();
	boost::interprocess::interprocess_mutex *createSharedMemory(key_t *key, int *shmid, char **data, int id);


	MutexLinkedList *_Buffer;

	void doLock(int id);
	void doUnlock(int id);
protected:

	////////////////////////Shared Memory///////////////
	key_t key[4];
	int shmid[4];
	char *data[4];
	boost::interprocess::interprocess_mutex *mutex[4];
	////////////////////////////////////////////////////

	void run(); //this is the function that will be iterated indefinitely
};

} /* namespace beeCompress */

#endif /* SHAREDMEMORY_H_ */
