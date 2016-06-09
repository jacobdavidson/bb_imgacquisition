/*
 * SharedMemory.cpp
 *
 *  Created on: Jun 8, 2016
 *      Author: hauke
 */

#include "SharedMemory.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <iostream>
#include "settings/Settings.h"
#include "settings/ParamNames.h"
#include <iostream>
#include <cstdio>

namespace beeCompress {

SharedMemory::SharedMemory() {
	_Buffer = new beeCompress::MutexLinkedList();
}

SharedMemory::~SharedMemory() {

}

void SharedMemory::doLock(int id){
	if(mutex[id] == 0){
		mutex[id] = createSharedMemory(&key[id], &shmid[id], &data[id],id);
	}
	else{
		//mutex[id]->lock();
		int i;
		for (i=0; i<20; i++){
			if(mutex[id]->try_lock()) break;

			usleep(50000); // wait 50ms
		}
		if (i==20){
			SettingsIAC* set 		= SettingsIAC::getInstance();
			EncoderQualityConfig cfg= set->getBufferConf(id,0);
			int 		width 		= cfg.width;
			int 		height 		= cfg.height;
			int 		lockpos 	= height*width ;
			mutex[id]				= new(data[id]+lockpos) boost::interprocess::interprocess_mutex();
			mutex[id]->lock();

			std::cout << "Created new lock to prevent starvation."<<std::endl;
		}
		//else
		//	std::cout << "locked"<<std::endl;
	}
}

void SharedMemory::doUnlock(int id){
	if(mutex[id] == 0){
		std::cout << "Error: Unlocking null pointer mutex "<<id << std::endl;
	}
	else{
		mutex[id]->unlock();
	}
}

void SharedMemory::run()
{

	while(true){
		//Wait until there is a new image available (done by pop)
		std::shared_ptr<beeCompress::ImageBuffer> imgptr = _Buffer->pop();
		beeCompress::ImageBuffer *img = imgptr.get();
		//std::cout << img->width * img->height << std::endl;

		doLock(img->camid);
		memcpy( data[img->camid], img->data, img->width * img->height);
		doUnlock(img->camid);
	}

	/* detach from the segment: */
	if (shmdt(data) == -1) {
		perror("shmdt");
		exit(1);
	}
}

boost::interprocess::interprocess_mutex *SharedMemory::createSharedMemory(key_t *key, int *shmid, char **data, int id){

	SettingsIAC* set 		= SettingsIAC::getInstance();
	EncoderQualityConfig cfg= set->getBufferConf(id,0);
	int 		width 		= cfg.width;
	int 		height 		= cfg.height;
	int 		lockpos 	= height*width ; //32 is w,h,camid ; 64 is timestamp
	int 		memsize 	= height*width + 32 + 64+sizeof(boost::interprocess::interprocess_mutex);

	std::string ftopkFile 	= "memory"+std::to_string(id)+".txt";
	/* make the key: */
	if ((*key = ftok(ftopkFile.c_str(), 'R')) == -1) /*Here the file must exist */
	{
		perror("ftok");
		exit(1);
	}

	/*  create the segment: */
	if ((*shmid = shmget(*key, memsize, 0644 | IPC_CREAT)) == -1) {
		perror("shmget");
		exit(1);
	}

	/* attach to the segment to get a pointer to it: */
	*data = shmat(*shmid, (void *)0, 0);
	if (*data == (char *)(-1)) {
		perror("shmat");
		exit(1);
	}

	boost::interprocess::interprocess_mutex *mutex = new((*data)+lockpos) boost::interprocess::interprocess_mutex();
	return mutex;
}

} /* namespace beeCompress */
