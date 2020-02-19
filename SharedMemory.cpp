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

void SharedMemory::doLock(int id) {
    if (_mutex[id] == 0) {
        _mutex[id] = createSharedMemory(&_key[id], &_shmid[id], &_data[id],id);
    }
    else {
        int i;
        for (i=0; i<20; i++) {
            if (_mutex[id]->try_lock()) {
                break;
            }

            usleep(50000); // wait 50ms
        }
        if (i==20) {
            SettingsIAC *set        = SettingsIAC::getInstance();
            EncoderQualityConfig cfg= set->getBufferConf(id,0);
            int width               = cfg.width;
            int height              = cfg.height;
            int lockpos             = height*width ;
            _mutex[id]              = new(_data[id]+lockpos) boost::interprocess::interprocess_mutex();
            _mutex[id]->lock();

            std::cout << "Created new lock to prevent starvation."<<std::endl;
        }
        //else
        //  std::cout << "locked"<<std::endl;
    }
}

void SharedMemory::doUnlock(int id) {
    if (_mutex[id] == 0) {
        std::cout << "Error: Unlocking null pointer mutex "<<id << std::endl;
    }
    else {
        _mutex[id]->unlock();
    }
}

void SharedMemory::run()
{

    while (true) {
        //Wait until there is a new image available (done by pop)
        std::shared_ptr<beeCompress::ImageBuffer> imgptr = _Buffer->pop();
        beeCompress::ImageBuffer *img = imgptr.get();

        doLock(img->camid);
        memcpy(_data[img->camid], img->data, img->width * img->height);
        doUnlock(img->camid);
    }

    /* detach from the segment: */
    if (shmdt(_data) == -1) {
        perror("shmdt");
        exit(1);
    }
}

boost::interprocess::interprocess_mutex *SharedMemory::createSharedMemory(key_t *key, int *shmid, char **data, int id) {

    SettingsIAC *set        = SettingsIAC::getInstance();
    EncoderQualityConfig cfg= set->getBufferConf(id,0);
    int         width       = cfg.width;
    int         height      = cfg.height;
    int         lockpos     = height*width ; //32 is w,h,camid ; 64 is timestamp
    int         memsize     = height*width + 32 + 64+sizeof(boost::interprocess::interprocess_mutex);

    std::string ftopkFile   = "memory"+std::to_string(id)+".txt";
    /* make the key: */
    if ((*key = ftok(ftopkFile.c_str(), 'R')) == -1) /*Here the file must exist */
    {
        perror("ftok");
        exit(1);
    }

    /*  create the segment: */
    if ((*shmid = shmget(*key, memsize, 0777 | IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }

    /* attach to the segment to get a pointer to it: */
    *data = static_cast<char*>(shmat(*shmid, (void *)0, 0));
    if (*data == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }
    assert((int64_t)(*data) != -1L);
    boost::interprocess::interprocess_mutex *mutex = new((*data)+lockpos) boost::interprocess::interprocess_mutex();
    return mutex;
}

} /* namespace beeCompress */
