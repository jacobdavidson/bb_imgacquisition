/*
 * NvEncGlue.cpp
 *
 *  Created on: Nov 6, 2015
 *      Author: hauke
 */

#include "NvEncGlue.h"
#include "nvenc/NvEncoder.h"
#include <time.h>

#include <unistd.h> //sleep
#include <iostream>

namespace beeCompress {

std::string NvEncGlue::getTimestamp(){
	struct			tm * timeinfo;
	time_t rawtime;
	char			timeresult[15];
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	//timeinfo = localtime(&secs);  //converts the time in seconds to local time
	// string with local time info
	sprintf(timeresult, "%d%.2d%.2d%.2d%.2d%.2d%",
			timeinfo -> tm_year + 1900,
			timeinfo -> tm_mon  + 1,
			timeinfo -> tm_mday,
			timeinfo -> tm_hour,
			timeinfo -> tm_min,
			timeinfo -> tm_sec);
	std::string r(timeresult);
	return r;
}

void NvEncGlue::run(){

	int rcmode=6;
	int preset=2;
	int qp=20;
	int bitrate=1000000;
	double elapsedTimeP,avgtimeP;
	int totalFrames=30;

	while(1){
		CNvEncoder enc;
		//enc.EncodeMain(int rcmode, int preset, int qp, int bitrate, double *elapsedTimeP, double *avgtimeP, beeCompress::MutexRingbuffer *buffer, FILE *f, int totalFrames);

		FILE *f;
		std::string timestamp = getTimestamp();
		f=fopen("out/test.avi","wb");
		enc.EncodeMain(rcmode, preset, qp, bitrate, &elapsedTimeP, &avgtimeP, &_Ring1, f, totalFrames);
		fclose(f);
		exit(0);
		break;

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
	// TODO Auto-generated constructor stub

}

NvEncGlue::~NvEncGlue() {
	// TODO Auto-generated destructor stub
}

} /* namespace beeCompress */
