#pragma once

#include <string> // std::string

namespace CONFIGPARAM {
static const std::string CONFIGURATION_FILE = "./configImAcq.json";
}

namespace IMACQUISITION {

	namespace BUFFERCONF{
	static const std::string CAMID 					= "CAMID";
	static const std::string ISPREVIEW 				= "ISPREVIEW";
	static const std::string BITRATE 				= "BITRATE";
	static const std::string RCMODE 				= "RCMODE";
	static const std::string QP		 				= "QP";
	static const std::string PRESET 				= "PRESET";
	static const std::string VIDEO_WIDTH 			= "VIDEO_WIDTH";
	static const std::string VIDEO_HEIGHT 			= "VIDEO_HEIGHT";
	static const std::string FRAMESPERVIDEO			= "FRAMESPERVIDEO";
	static const std::string FPS 					= "FPS";
	}

static const std::string BUFFER						= "IMACQUISITION.BUFFER";

static const std::string DO_PREVIEWS 				= "IMACQUISITION.DO_PREVIEWS";
static const std::string LOGDIR 					= "IMACQUISITION.LOGDIR";
static const std::string IMDIR 						= "IMACQUISITION.IMDIR";
static const std::string IMDIRPREVIEW 				= "IMACQUISITION.IMDIRPREVIEW";
static const std::string EXCHANGEDIR 				= "IMACQUISITION.EXCHANGEDIR";
static const std::string EXCHANGEDIRPREVIEW 		= "IMACQUISITION.EXCHANGEDIRPREVIEW";
}


