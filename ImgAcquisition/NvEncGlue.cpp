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

	//For file writing
	char 		filepath[512];
	//For logging encoding times
	double 		elapsedTimeP,avgtimeP;

	//Encoder may be reused. Potentially saves time.
	CNvEncoder 	enc;

	while(1){

		//Select a buffer to work on
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

		//Create assemble file name and create a file handle to pass the encoder.
		FILE *f;	//Target video file
		FILE *lck;	//lock file, so no one grabs the unfinished video
		std::string timestamp = getTimestamp();

		sprintf(filepath, imdir.c_str(),
				currentCam,currentCam,timestamp.c_str(),0);

		std::string lockfile = filepath;
		lockfile			+= ".lck";
		std::cout << "Creating: " << filepath << " with lock " << lockfile << std::endl;

		lck	=	fopen(lockfile.c_str(),"wb");
		f	=	fopen(filepath,"wb");

		//encode the frames in the buffer using given configuration
		enc.EncodeMain(rcmode, preset, qp, bitrate, &elapsedTimeP, &avgtimeP, currentCamBuffer, f, totalFrames, fps, width, height);

		//Always be a good citizen and close your file handles.
		fclose(f);
		fclose(lck);

		//Remove the lockfile, so others will be allowed to grab the video
		remove( lockfile.c_str() );
	}
}

NvEncGlue::NvEncGlue() {

	SettingsIAC *set 			= SettingsIAC::getInstance();

	//Grab buffer size from json config and initialize buffers.
	int 		buffersize		= set->getValueOfParam<int>(IMACQUISITION::BUFFERSIZE);
	_Ring1 						= new MutexRingbuffer(buffersize);
	_Ring2 						= new MutexRingbuffer(buffersize);

}

NvEncGlue::~NvEncGlue() {
	// TODO Auto-generated destructor stub
}

} /* namespace beeCompress */
