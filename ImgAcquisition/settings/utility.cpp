/*
 * utility.cpp
 *
 *  Created on: Nov 11, 2015
 *      Author: hauke
 */

#include "utility.h"

#include <time.h>
#if __linux__
#include <sys/time.h>
#else
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "windows.h"
#endif

/*
 * Timestamp creation from system clock
 */
std::string getTimestamp(){
#if __linux__
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
#else
	char		timeresult[20];
	SYSTEMTIME	stime;
	//structure to store system time (in usual time format)
	FILETIME	ltime;
	//structure to store local time (local time in 64 bits)
	FILETIME	ftTimeStamp;
	GetSystemTimeAsFileTime(&ftTimeStamp); //Gets the current system time

	FileTimeToLocalFileTime(&ftTimeStamp, &ltime);//convert in local time and store in ltime
	FileTimeToSystemTime(&ltime, &stime);//convert in system time and store in stime
	
	sprintf(timeresult, "%d%.2d%.2d%.2d%.2d%.2d_%06d",
		stime.wYear,
		stime.wMonth,
		stime.wDay,
		stime.wHour,
		stime.wMinute,
		stime.wSecond,
		stime.wMilliseconds);
	std::string r(timeresult);
	return r;
#endif
}
