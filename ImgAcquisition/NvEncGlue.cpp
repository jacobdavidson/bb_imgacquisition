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

	SettingsIAC *set 		= SettingsIAC::getInstance();

	std::string imdir 			= set->getValueOfParam<std::string>(IMACQUISITION::IMDIR);
	std::string imdirprev		= set->getValueOfParam<std::string>(IMACQUISITION::IMDIRPREVIEW);
	std::string exchangedir		= set->getValueOfParam<std::string>(IMACQUISITION::EXCHANGEDIR);
	std::string exchangedirprev	= set->getValueOfParam<std::string>(IMACQUISITION::EXCHANGEDIRPREVIEW);

	//For logging encoding times
	double 		elapsedTimeP,avgtimeP;

	//Encoder may be reused. Potentially saves time.
	CNvEncoder 	enc;
	EncoderQualityConfig 	cfgC1 = set->getBufferConf(_CamBuffer1,0);
	EncoderQualityConfig 	cfgC2 = set->getBufferConf(_CamBuffer2,0);
	EncoderQualityConfig 	cfgP1 = set->getBufferConf(_CamBuffer1,1);
	EncoderQualityConfig 	cfgP2 = set->getBufferConf(_CamBuffer2,1);

	while(1){

		//Select a buffer to work on
		long long unsigned int c1 = _Buffer1->size() * (long long unsigned int)(cfgC1.width * cfgC1.height);
		long long unsigned int c2 = _Buffer2->size() * (long long unsigned int)(cfgC2.width * cfgC2.height);
		long long unsigned int c1p = _Buffer1_preview->size() * (long long unsigned int)(cfgP1.width * cfgP1.height);
		long long unsigned int c2p = _Buffer2_preview->size() * (long long unsigned int)(cfgP2.width * cfgP2.height);
		int currentCam=0;
		MutexBuffer *currentCamBuffer;
		MutexBuffer *currentPreviewBuffer;
		EncoderQualityConfig encCfg;
		EncoderQualityConfig encCfgPrev;

		//std::cout << "Sizes:" << c1 << " - " << c2 << " - " << c1p << " - " << c2p << std::endl;

		long long unsigned int maxSize = max(max(max(c1,c2),c1p),c2p);

		if (maxSize == 0){
			usleep(500);
			continue;
		}

		if(c1>=maxSize){
			currentCamBuffer 		= _Buffer1;
			currentPreviewBuffer 	= _Buffer1_preview;
			currentCam 				= _CamBuffer1;
			encCfg 					= cfgC1;
			encCfgPrev				= cfgP1;
			std::cout << "Chosen: 1"<<std::endl;
		}
		else if(c1p>=maxSize){
			currentCamBuffer 		= _Buffer1_preview;
			currentPreviewBuffer 	= NULL;
			currentCam 				= _CamBuffer1;
			encCfg 					= cfgP1;
			encCfgPrev				= cfgP1;
			std::cout << "Chosen: 2"<<std::endl;
		}
		else if(c2>=maxSize){
			currentCamBuffer 		= _Buffer2;
			currentPreviewBuffer 	= _Buffer2_preview;
			currentCam 				= _CamBuffer2;
			encCfg 					= cfgC2;
			encCfgPrev				= cfgP2;
			std::cout << "Chosen: 3"<<std::endl;
		}
		else if(c2p>=maxSize){
			currentCamBuffer 		= _Buffer2_preview;
			currentPreviewBuffer 	= NULL;
			currentCam 				= _CamBuffer2;
			encCfg 					= cfgP2;
			encCfgPrev				= cfgP2;
			std::cout << "Chosen: 4"<<std::endl;
		}

		std::string dir = imdir;
		std::string exdir = exchangedir;
		if (currentPreviewBuffer == NULL){
			dir = imdirprev;
			exdir = exchangedirprev;
		}
		beeCompress::writeHandler wh(dir,currentCam,exdir);
		//encode the frames in the buffer using given configuration
		enc.EncodeMain(&elapsedTimeP, &avgtimeP, currentCamBuffer, currentPreviewBuffer, &wh, encCfg, encCfgPrev);

	}
}

NvEncGlue::NvEncGlue() {

	SettingsIAC *set 			= SettingsIAC::getInstance();

	//Grab buffer size from json config and initialize buffers.
	//int 		buffersize			= set->getValueOfParam<int>(IMACQUISITION::BUFFERSIZE);
	_Buffer1 						= new beeCompress::MutexLinkedList();
	_Buffer2 						= new beeCompress::MutexLinkedList();
	_Buffer1_preview 				= new beeCompress::MutexLinkedList();
	_Buffer2_preview 				= new beeCompress::MutexLinkedList();
	_CamBuffer1						= -1;
	_CamBuffer2						= -1;

}

NvEncGlue::~NvEncGlue() {
	// TODO Auto-generated destructor stub
}

} /* namespace beeCompress */
