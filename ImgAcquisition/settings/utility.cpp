/*
 * utility.cpp
 *
 *  Created on: Nov 11, 2015
 *      Author: hauke
 */

#include "utility.h"

#include <time.h>

std::string getTimestamp(){
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
