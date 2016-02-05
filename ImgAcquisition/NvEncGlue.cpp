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

	//grab the configuration from the json file
	EncoderQualityConfig encCfgHiRes;
	encCfgHiRes.rcmode			= set->getValueOfParam<int>(IMACQUISITION::RCMODE);
	encCfgHiRes.preset			= set->getValueOfParam<int>(IMACQUISITION::PRESET);
	encCfgHiRes.qp				= set->getValueOfParam<int>(IMACQUISITION::QP);
	encCfgHiRes.bitrate			= set->getValueOfParam<int>(IMACQUISITION::BITRATE);
	encCfgHiRes.totalFrames		= set->getValueOfParam<int>(IMACQUISITION::FRAMESPERVIDEO);
	encCfgHiRes.width			= set->getValueOfParam<int>(IMACQUISITION::VIDEO_WIDTH);
	encCfgHiRes.height			= set->getValueOfParam<int>(IMACQUISITION::VIDEO_HEIGHT);
	encCfgHiRes.fps				= set->getValueOfParam<int>(IMACQUISITION::FPS);
	EncoderQualityConfig encCfgPreview;
	encCfgPreview.rcmode		= set->getValueOfParam<int>(IMACQUISITION::RCMODE);
	encCfgPreview.preset		= set->getValueOfParam<int>(IMACQUISITION::PRESET);
	encCfgPreview.qp			= 30;
	encCfgPreview.bitrate		= set->getValueOfParam<int>(IMACQUISITION::BITRATE);
	encCfgPreview.totalFrames	= set->getValueOfParam<int>(IMACQUISITION::FRAMESPERVIDEO);
	encCfgPreview.width			= set->getValueOfParam<int>(IMACQUISITION::VIDEO_WIDTH) / 2;
	encCfgPreview.height		= set->getValueOfParam<int>(IMACQUISITION::VIDEO_HEIGHT) / 2;
	encCfgPreview.fps			= set->getValueOfParam<int>(IMACQUISITION::FPS);
	std::string imdir 		= set->getValueOfParam<std::string>(IMACQUISITION::IMDIR);
	std::string imdirprev	= set->getValueOfParam<std::string>(IMACQUISITION::IMDIRPREVIEW);

	//For logging encoding times
	double 		elapsedTimeP,avgtimeP;

	//Encoder may be reused. Potentially saves time.
	CNvEncoder 	enc;

	while(1){

		//Select a buffer to work on
		long long unsigned int c1 = _Buffer1->size() * (long long unsigned int)(encCfgHiRes.width * encCfgHiRes.height);
		long long unsigned int c2 = _Buffer2->size() * (long long unsigned int)(encCfgHiRes.width * encCfgHiRes.height);
		long long unsigned int c1p = _Buffer1_preview->size() * (long long unsigned int)(encCfgPreview.width * encCfgPreview.height);
		long long unsigned int c2p = _Buffer2_preview->size() * (long long unsigned int)(encCfgPreview.width * encCfgPreview.height);
		int currentCam=0;
		MutexBuffer *currentCamBuffer;
		MutexBuffer *currentPreviewBuffer;
		EncoderQualityConfig encCfg;

		std::cout << "Sizes:" << c1 << " - " << c2 << " - " << c1p << " - " << c2p << std::endl;

		long long unsigned int maxSize = max(max(max(c1,c2),c1p),c2p);
		if(c1>=maxSize){
			currentCamBuffer = _Buffer1;
			currentPreviewBuffer = _Buffer1_preview;
			currentCam = _CamBuffer1;
			encCfg = encCfgHiRes;
			std::cout << "Chosen: 1"<<std::endl;
		}
		else if(c1p>=maxSize){
			currentCamBuffer = _Buffer1_preview;
			currentPreviewBuffer = NULL;
			currentCam = _CamBuffer1;
			encCfg = encCfgPreview;
			std::cout << "Chosen: 2"<<std::endl;
		}
		else if(c2>=maxSize){
			currentCamBuffer = _Buffer2;
			currentPreviewBuffer = _Buffer2_preview;
			currentCam = _CamBuffer2;
			encCfg = encCfgHiRes;
			std::cout << "Chosen: 3"<<std::endl;
		}
		else if(c2p>=maxSize){
			currentCamBuffer = _Buffer2_preview;
			currentPreviewBuffer = NULL;
			currentCam = _CamBuffer2;
			encCfg = encCfgPreview;
			std::cout << "Chosen: 4"<<std::endl;
		}

		std::string dir = imdir;
		if (currentPreviewBuffer == NULL){
			dir = imdirprev;
		}
		beeCompress::writeHandler wh(dir,currentCam);
		//encode the frames in the buffer using given configuration
		enc.EncodeMain(&elapsedTimeP, &avgtimeP, currentCamBuffer, currentPreviewBuffer, &wh, encCfg);

	}
}

NvEncGlue::NvEncGlue() {

	SettingsIAC *set 			= SettingsIAC::getInstance();

	//Grab buffer size from json config and initialize buffers.
	int 		buffersize			= set->getValueOfParam<int>(IMACQUISITION::BUFFERSIZE);
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
