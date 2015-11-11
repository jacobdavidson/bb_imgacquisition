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

namespace beeCompress {

void NvEncGlue::run(){

	SettingsIAC *set = SettingsIAC::getInstance();

	int 		rcmode			= set->getValueOfParam<int>(IMACQUISITION::RCMODE);
	int 		preset			= set->getValueOfParam<int>(IMACQUISITION::PRESET);
	int 		qp				= set->getValueOfParam<int>(IMACQUISITION::QP);
	int 		bitrate			= set->getValueOfParam<int>(IMACQUISITION::BITRATE);
	int 		totalFrames		= set->getValueOfParam<int>(IMACQUISITION::FRAMESPERVIDEO);
	std::string imdir 			= set->getValueOfParam<std::string>(IMACQUISITION::IMDIR);
	char 		filepath[512];
	double 		elapsedTimeP,avgtimeP;

	while(1){
		CNvEncoder enc;
		//enc.EncodeMain(int rcmode, int preset, int qp, int bitrate, double *elapsedTimeP, double *avgtimeP, beeCompress::MutexRingbuffer *buffer, FILE *f, int totalFrames);

		int c1 = _Ring1->_Count;
		int c2 = _Ring2->_Count;
		int currentCam=0;
		MutexRingbuffer *currentCamBuffer;

		if(c1>=c2){
			currentCamBuffer = _Ring1;
			currentCam = _CamRing1;
		}else{
			currentCamBuffer = _Ring2;
			currentCam = _CamRing2;
		}

		FILE *f;
		std::string timestamp = getTimestamp();
		sprintf(filepath, imdir.c_str(),
				currentCam,currentCam,timestamp.c_str(),0);
		std::cout << "Creating: " << filepath << std::endl;
		f=fopen(filepath,"wb");
		enc.EncodeMain(rcmode, preset, qp, bitrate, &elapsedTimeP, &avgtimeP, currentCamBuffer, f, totalFrames);
		fclose(f);

		/*
		usleep(5*1000*1000);

		FlyCapture2::Image* img;
		int c=0;
		while((img=_Ring1.back()) != NULL){
			c++;
			// ...
		}
		std::cout << "Processed " << c << " images "<<std::endl;
		*/
	}
}

NvEncGlue::NvEncGlue() {
	SettingsIAC *set 			= SettingsIAC::getInstance();
	int 		buffersize		= set->getValueOfParam<int>(IMACQUISITION::BUFFERSIZE);
	_Ring1 						= new MutexRingbuffer(buffersize);
	_Ring2 						= new MutexRingbuffer(buffersize);

}

NvEncGlue::~NvEncGlue() {
	// TODO Auto-generated destructor stub
}

} /* namespace beeCompress */
