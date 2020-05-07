// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

class writeHandler
{

    // All public policy
public:
    //! Target video file
    FILE* _video;

    //! lock file, so no one grabs the unfinished video
    FILE* _lock;

    //! text file holding the names of the frames
    FILE* _frames;

    //! Lockfile which is created in temp dirs. Deprecated
    std::string _lockfile;

    //! Video file to create
    std::string _videofile;

    //! Frames textfile to create
    std::string _framesfile;

    //! The path to the tmp dir
    std::string _basename;

    //! Timestamp of the start of the video
    std::string _firstTimestamp;

    //! Timestamp of the end of the video
    std::string _lastTimestamp;

    //! The path to the out dir
    std::string _exchangedir;

    //! The camera ID
    std::string _camId;

    /**
     * @brief Writes a line to the textfile
     *
     * @param Timestamp of the line to write
     */
    void log(std::string timestamp);

    /**
     * @brief Constructor. Assembles pathes and creates file handles.
     *
     * @param Sets the path to the tmp dir
     * @param Sets the camera ID
     * @param Sets the path to the out dir
     */
    writeHandler(std::string imdir, std::string currentCam, std::string exchangedir);

    /**
     * @brief Destructor. Finalizes writing.
     *
     * Closes file handles, moves files to the exchangedirs and
     * deletes eventual locks.
     */
    virtual ~writeHandler();
};
