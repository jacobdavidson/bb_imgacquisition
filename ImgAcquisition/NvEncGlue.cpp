/*
 * NvEncGlue.cpp
 *
 *  Created on: Nov 6, 2015
 *      Author: hauke
 */

#include <time.h>

#if HAVE_UNISTD_H
#include <unistd.h> //sleep
#else
#include <stdint.h>
#endif
#include <iostream>
#include "settings/utility.h"
#include "settings/Settings.h"
//The order is important!
#include "NvEncGlue.h"
#include "nvenc/NvEncoder.h"
#include "writeHandler.h"

namespace beeCompress {

void NvEncGlue::run(){

	SettingsIAC *set = SettingsIAC::getInstance();

	//grab the configuration from the json file
	int 		rcmode			= set->getValueOfParam<int>(IMACQUISITION::RCMODE);
	int 		preset			= set->getValueOfParam<int>(IMACQUISITION::PRESET);
	int 		qp				= set->getValueOfParam<int>(IMACQUISITION::QP);
	int 		bitrate			= set->getValueOfParam<int>(IMACQUISITION::BITRATE);
	int 		totalFrames		= set->getValueOfParam<int>(IMACQUISITION::FRAMESPERVIDEO);
	int 		width			= set->getValueOfParam<int>(IMACQUISITION::VIDEO_WIDTH);
	int 		height			= set->getValueOfParam<int>(IMACQUISITION::VIDEO_HEIGHT);
	int 		fps				= set->getValueOfParam<int>(IMACQUISITION::FPS);
	std::string imdir 			= set->getValueOfParam<std::string>(IMACQUISITION::IMDIR);

	//For logging encoding times
	double 		elapsedTimeP,avgtimeP;

	//Encoder may be reused. Potentially saves time.
	CNvEncoder 	enc;

	while(1){

		//Select a buffer to work on
		int c1 = _Buffer1->size();
		int c2 = _Buffer1->size();
		int currentCam=0;
		MutexBuffer *currentCamBuffer;

		if(c1>=c2){
			currentCamBuffer = _Buffer1;
			currentCam = _CamBuffer1;
		}else{
			currentCamBuffer = _Buffer2;
			currentCam = _CamBuffer2;
		}

		beeCompress::writeHandler wh(imdir,currentCam);
		//encode the frames in the buffer using given configuration
		enc.EncodeMain(rcmode, preset, qp, bitrate, &elapsedTimeP, &avgtimeP, currentCamBuffer, &wh, totalFrames, fps, width, height);

	}
}

NvEncGlue::NvEncGlue() {

	SettingsIAC *set 			= SettingsIAC::getInstance();

	//Grab buffer size from json config and initialize buffers.
	int 		buffersize		= set->getValueOfParam<int>(IMACQUISITION::BUFFERSIZE);
	_Buffer1 						= new beeCompress::MutexLinkedList();
	_Buffer2 						= new beeCompress::MutexLinkedList();
	_CamBuffer1						= -1;
	_CamBuffer2						= -1;

}

NvEncGlue::~NvEncGlue() {
	// TODO Auto-generated destructor stub
}

} /* namespace beeCompress */
