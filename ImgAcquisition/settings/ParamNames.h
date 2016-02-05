#pragma once

#include <string> // std::string

namespace CONFIGPARAM {
static const std::string CONFIGURATION_FILE = "./configImAcq.json";
}

namespace IMACQUISITION {
static const std::string BITRATE 				= "IMACQUISITION.BITRATE";
static const std::string RCMODE 				= "IMACQUISITION.RCMODE";
static const std::string QP		 				= "IMACQUISITION.QP";
static const std::string PRESET 				= "IMACQUISITION.PRESET";
static const std::string VIDEO_WIDTH 			= "IMACQUISITION.VIDEO_WIDTH";
static const std::string VIDEO_HEIGHT 			= "IMACQUISITION.VIDEO_HEIGHT";
static const std::string FRAMESPERVIDEO			= "IMACQUISITION.FRAMESPERVIDEO";
static const std::string BUFFERSIZE				= "IMACQUISITION.BUFFERSIZE";
static const std::string FPS 					= "IMACQUISITION.FPS";
static const std::string LOGDIR 				= "IMACQUISITION.LOGDIR";
static const std::string IMDIR 					= "IMACQUISITION.IMDIR";
static const std::string IMDIRPREVIEW 			= "IMACQUISITION.IMDIRPREVIEW";
}


