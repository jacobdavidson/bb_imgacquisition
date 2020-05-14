// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * writeHandler.cpp
 *
 *  Created on: Feb 4, 2016
 *      Author: hauke
 */

#include "writeHandler.h"
#include "settings/utility.h"
#include <sstream>
#include <iostream>
#include <boost/filesystem.hpp>

writeHandler::writeHandler(std::string imdir, std::string currentCam, std::string edir)
: _skipFinalization{false}
{

    // Create assemble file name and create a file handle to pass the encoder.
    std::string timestamp = get_utc_time();
    _basename             = imdir;
    _camId                = currentCam;

    // For file writing
    char filepath[512];
    char exdirFilepath[512];

    sprintf(filepath,
            _basename.c_str(),
            _camId.c_str(),
            _camId.c_str(),
            timestamp.c_str(),
            timestamp.c_str(),
            0);
    sprintf(exdirFilepath, edir.c_str(), _camId.c_str(), 0);

    boost::filesystem::create_directories({filepath});
    std::string tmp = filepath;
    _exchangedir    = exdirFilepath;
    _videofile      = tmp + ".mp4";
    _framesfile     = tmp + ".txt";

    _frames = fopen(_framesfile.c_str(), "wb");
    if (_frames == nullptr)
    {
        std::cerr << "Timestamps file could not be opened!" << std::endl;
        assert(false);
        exit(1);
    }
}

void writeHandler::log(std::string timestamp)
{

    if (_firstTimestamp == "")
    {
        _firstTimestamp = timestamp;
    }
    _lastTimestamp = timestamp;
    std::stringstream line;
    line << "Cam_" << _camId << "_" << timestamp << "\n";
    fwrite(line.str().c_str(), sizeof(char), line.str().size(), _frames);
    fflush(_frames);
}

writeHandler::~writeHandler()
{
    if (_frames)
        fclose(_frames);

    if (_skipFinalization)
    {
        return;
    }

    // For filling the basepath
    char filepath[512];

    // assemble final file name
    sprintf(filepath,
            _basename.c_str(),
            _camId.c_str(),
            _camId.c_str(),
            _firstTimestamp.c_str(),
            _lastTimestamp.c_str(),
            0);

    // Rename the temporary files to their final names:
    std::string             tmp           = filepath;
    std::string             newvideofile  = tmp + ".mp4";
    std::string             newframesfile = tmp + ".txt";
    boost::filesystem::path video(newvideofile);
    newvideofile = _exchangedir + video.filename().string();
    boost::filesystem::path frames(newframesfile);
    newframesfile = _exchangedir + frames.filename().string();

    {
        using namespace boost::system;
        using namespace boost::filesystem;

        try
        {
            const auto outputDirectory = path{newvideofile}.parent_path();
            if (!exists(outputDirectory))
            {
                create_directories(outputDirectory);
                permissions(outputDirectory, owner_all | group_all | others_read | others_exe);
            }

            rename(_videofile, newvideofile);
            permissions(newvideofile, owner_read | owner_write | group_read | group_write | others_read | others_write);
        }
        catch (const filesystem_error& e)
        {
            std::cerr << e.what() << std::endl;
        }

        try
        {
            const auto outputDirectory = path{newframesfile}.parent_path();
            if (!exists(outputDirectory))
            {
                create_directories(outputDirectory);
                permissions(outputDirectory, owner_all | group_all | others_read | others_exe);
            }

            rename(_framesfile, newframesfile);
            permissions(newframesfile, owner_read | owner_write | group_read | group_write | others_read | others_write);
        }
        catch (const filesystem_error& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
}
