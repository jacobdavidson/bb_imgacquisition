/*
 * utility.cpp
 *
 *  Created on: Nov 11, 2015
 *      Author: hauke
 */

#include "utility.h"

#include <time.h>
#include <sys/time.h>

/*
 * Timestamp creation from system clock
 */
std::string getTimestamp(){
	struct			tm * timeinfo;
	char			timeresult[15];
	struct timeval 	tv;
	struct timezone tz;
	struct tm 		*tm;

	gettimeofday(&tv, &tz);
	timeinfo=localtime(&tv.tv_sec);


	sprintf(timeresult, "%d%.2d%.2d%.2d%.2d%.2d_%06d",
			timeinfo -> tm_year + 1900,
			timeinfo -> tm_mon  + 1,
			timeinfo -> tm_mday,
			timeinfo -> tm_hour,
			timeinfo -> tm_min,
			timeinfo -> tm_sec,
			tv		 .	tv_usec);
	std::string r(timeresult);
	return r;
}
