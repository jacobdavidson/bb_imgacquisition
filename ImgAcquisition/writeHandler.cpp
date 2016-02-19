/*
 * writeHandler.cpp
 *
 *  Created on: Feb 4, 2016
 *      Author: hauke
 */

#include "writeHandler.h"
#include "settings/utility.h"
#include <sstream>
#include <boost/filesystem.hpp>

namespace beeCompress {

writeHandler::writeHandler(std::string imdir, int currentCam, std::string edir) {
	basename = imdir;

	//For file writing
	char 		filepath[512];
	char 		exdirFilepath[512];
	camId = currentCam;

	//Create assemble file name and create a file handle to pass the encoder.
	std::string timestamp = getTimestamp();

	sprintf(filepath, basename.c_str(),
			camId,camId,timestamp.c_str(),camId,timestamp.c_str(),0);
	sprintf(exdirFilepath, edir.c_str(),
			camId,0);
	exchangedir = exdirFilepath;

	std::string tmp = filepath;
	lockfile = tmp + ".lck";
	videofile = tmp + ".avi";
	framesfile = tmp + ".txt";

	//Open for writing
	lock	=	fopen(lockfile.c_str(),"wb");
	video	=	fopen(videofile.c_str(),"wb");
	frames	=	fopen(framesfile.c_str(),"wb");
}

void writeHandler::log(std::string timestamp){

	if (firstTimestamp=="") firstTimestamp = timestamp;
	lastTimestamp = timestamp;
	std::stringstream line;
	line << "Cam_" << camId << "_"<<timestamp<<"\n";
	fwrite(line.str().c_str(),sizeof(char), line.str().size(),frames);
	fflush(frames);
}

writeHandler::~writeHandler() {
	//Always be a good citizen and close your file handles.
	fclose(video);
	fclose(lock);
	fclose(frames);

	//For filling the basepath
	char filepath[512];

	//assemble final file name
	sprintf(filepath, basename.c_str(),
			camId,camId,firstTimestamp.c_str(),camId,lastTimestamp.c_str(),0);

	//Rename the temporary files to their final names:
	std::string tmp = filepath;
	std::string newvideofile = tmp + ".avi";
	std::string newframesfile = tmp + ".txt";
	boost::filesystem::path video(newvideofile);
	newvideofile = exchangedir+video.filename().string();
	boost::filesystem::path frames(newframesfile);
	newframesfile = exchangedir+frames.filename().string();

	rename(videofile.c_str(),newvideofile.c_str());
	rename(framesfile.c_str(),newframesfile.c_str());

	//Remove the lockfile, so others will be allowed to grab the video
	remove( lockfile.c_str() );
}

} /* namespace beeCompress */
