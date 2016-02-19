/*
 * writeHandler.h
 *
 *  Created on: Feb 4, 2016
 *      Author: hauke
 */

#ifndef WRITEHANDLER_H_
#define WRITEHANDLER_H_
#include <string>

namespace beeCompress {

class writeHandler {
public:

	FILE 		*video;	//Target video file
	FILE 		*lock;		//lock file, so no one grabs the unfinished video
	FILE		*frames;	//text file holding the names of the frames

	std::string lockfile;
	std::string videofile;
	std::string framesfile;
	std::string basename;
	std::string firstTimestamp;
	std::string lastTimestamp;
	std::string exchangedir;

	int 		camId;

	void log(std::string timestamp);
	writeHandler(std::string imdir, int currentCam, std::string exchangedir);
	virtual ~writeHandler();
};

} /* namespace beeCompress */

#endif /* WRITEHANDLER_H_ */
