// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

class WriteHandler
{

    // All public policy
public:
    //! text file holding the names of the frames
    FILE* _frames;

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

    //! Whether to skip moving the finished video file to out dir
    bool _skipFinalization;

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
    WriteHandler(std::string imdir, std::string currentCam, std::string exchangedir);

    virtual ~WriteHandler();
};
