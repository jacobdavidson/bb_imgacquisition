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

writeHandler::writeHandler(std::string imdir, int currentCam,
                           std::string edir) {

    //Create assemble file name and create a file handle to pass the encoder.
    std::string timestamp   = getTimestamp();
    _basename                = imdir;
    _camId                   = currentCam;

    //For file writing
    char filepath[512];
    char exdirFilepath[512];

    sprintf(filepath, _basename.c_str(), _camId, _camId, timestamp.c_str(),
            timestamp.c_str(), 0);
    sprintf(exdirFilepath, edir.c_str(), _camId, 0);

    std::string tmp = filepath;
    _exchangedir     = exdirFilepath;
    _lockfile        = tmp + ".lck";
    _videofile       = tmp + ".avi";
    _framesfile      = tmp + ".txt";

    //Open for writing
    _lock    = fopen(_lockfile.c_str(), "wb");
    _video   = fopen(_videofile.c_str(), "wb");
    _frames  = fopen(_framesfile.c_str(), "wb");
}

void writeHandler::log(std::string timestamp) {

    if (_firstTimestamp == "") {
        _firstTimestamp = timestamp;
    }
    _lastTimestamp = timestamp;
    std::stringstream line;
    line << "Cam_" << _camId << "_" << timestamp << "\n";
    fwrite(line.str().c_str(), sizeof(char), line.str().size(), _frames);
    fflush(_frames);
}

writeHandler::~writeHandler() {
    //Always be a good citizen and close your file handles.
    fclose(_video);
    fclose(_lock);
    fclose(_frames);

    //For filling the basepath
    char filepath[512];

    //assemble final file name
    sprintf(filepath, _basename.c_str(), _camId, _camId, _firstTimestamp.c_str(),
            _camId, _lastTimestamp.c_str(), 0);

    //Rename the temporary files to their final names:
    std::string tmp = filepath;
    std::string newvideofile = tmp + ".avi";
    std::string newframesfile = tmp + ".txt";
    boost::filesystem::path video(newvideofile);
    newvideofile = _exchangedir + video.filename().string();
    boost::filesystem::path frames(newframesfile);
    newframesfile = _exchangedir + frames.filename().string();

    rename(_videofile.c_str(), newvideofile.c_str());
    rename(_framesfile.c_str(), newframesfile.c_str());

    //Remove the lockfile, so others will be allowed to grab the video
    remove(_lockfile.c_str());
}

} /* namespace beeCompress */
