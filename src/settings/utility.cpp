/*
 * utility.cpp
 *
 *  Created on: Nov 11, 2015
 *      Author: hauke
 */

#include "utility.h"
#include "Settings.h"

#include <time.h>
#if __linux__
#include <sys/time.h>
#else
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "windows.h"
#endif

#include "boost/date_time/local_time/local_time.hpp"
#include "boost/date_time/c_local_time_adjustor.hpp"

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/date_time.hpp>

#include<cstdlib>

std::string get_utc_time() {
    return boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::universal_time())+"Z";
}

boost::posix_time::time_duration get_utc_offset() {
    using namespace boost::posix_time;

    // boost::date_time::c_local_adjustor uses the C-API to adjust a
    // moment given in utc to the same moment in the local time zone.
    typedef boost::date_time::c_local_adjustor<ptime> local_adj;

    const ptime utc_now = second_clock::universal_time();
    const ptime now = local_adj::utc_to_local(utc_now);

    return now - utc_now;
}

std::string get_utc_offset_string() {
    std::stringstream out;

    using namespace boost::posix_time;
//This is no memory leak!
    time_facet* tf = new time_facet();
    tf->time_duration_format("%+%H:%M");
    out.imbue(std::locale(out.getloc(), tf));

    out << get_utc_offset();

    return out.str();
}

/*
 * Timestamp creation from system clock
 */
std::string getTimestamp(){
#if __linux__
    struct          tm * timeinfo;
    char            timeresult[32];
    struct timeval  tv;
    struct timezone tz;
    struct tm       *tm;

    gettimeofday(&tv, &tz);
    timeinfo=localtime(&tv.tv_sec);


    sprintf(timeresult, "%d-%.2d-%.2dT%.2d:%.2d:%.2d.%03d%s",
            timeinfo -> tm_year + 1900,
            timeinfo -> tm_mon  + 1,
            timeinfo -> tm_mday,
            timeinfo -> tm_hour,
            timeinfo -> tm_min,
            timeinfo -> tm_sec,
            (tv.tv_usec/1000),
            get_utc_offset_string().c_str());
    std::string r(timeresult);
    return r;
#else
    char		timeresult[32];
    SYSTEMTIME	stime;
    //structure to store system time (in usual time format)
    FILETIME	ltime;
    //structure to store local time (local time in 64 bits)
    FILETIME	ftTimeStamp;
    GetSystemTimeAsFileTime(&ftTimeStamp); //Gets the current system time

    FileTimeToLocalFileTime(&ftTimeStamp, &ltime);//convert in local time and store in ltime
    FileTimeToSystemTime(&ltime, &stime);//convert in system time and store in stime

    sprintf(timeresult, "%d-%.2d-%.2dT%.2d:%.2d:%.2d.%03d%s",
            stime.wYear,
            stime.wMonth,
            stime.wDay,
            stime.wHour,
            stime.wMinute,
            stime.wSecond,
            stime.wMilliseconds,
            get_utc_offset_string().c_str());
    std::string r(timeresult);
    return r;
#endif
}
