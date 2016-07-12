#pragma once

#include <string> // std::string

namespace CONFIGPARAM {
static const std::string CONFIGURATION_FILE = "./configImAcq.json";
}

namespace IMACQUISITION {

	namespace BUFFERCONF{
	static const std::string CAMID 					= "CAMID";
	static const std::string ISPREVIEW 				= "ISPREVIEW";
	static const std::string SERIAL 				= "SERIAL";
	static const std::string ENABLED 				= "ENABLED";
	static const std::string BITRATE 				= "BITRATE";
	static const std::string RCMODE 				= "RCMODE";
	static const std::string QP		 				= "QP";
	static const std::string PRESET 				= "PRESET";
	static const std::string VIDEO_WIDTH 			= "VIDEO_WIDTH";
	static const std::string VIDEO_HEIGHT 			= "VIDEO_HEIGHT";
	static const std::string FRAMESPERVIDEO			= "FRAMESPERVIDEO";
	static const std::string FPS 					= "FPS";

	static const std::string OFFSETX				= "OFFSETX";
	static const std::string OFFSETY				= "OFFSETY";
	static const std::string HWBUFSIZE				= "HWBUFSIZE";
	static const std::string EXPOSUREONOFF			= "EXPOSUREONOFF";
	static const std::string EXPOSUREAUTO			= "EXPOSUREAUTO";
	static const std::string BRIGHTNESSONOFF		= "BRIGHTNESSONOFF";
	static const std::string BRIGHTNESSAUTO			= "BRIGHTNESSAUTO";
	static const std::string BRIGHTNESS				= "BRIGHTNESS";
	static const std::string SHUTTER				= "SHUTTER";
	static const std::string SHUTTERONOFF			= "SHUTTERONOFF";
	static const std::string SHUTTERAUTO			= "SHUTTERAUTO";
	static const std::string GAIN					= "GAIN";
	static const std::string GAINONOFF				= "GAINONOFF";
	static const std::string GAINAUTO				= "GAINAUTO";
	static const std::string WHITEBALANCE			= "WHITEBALANCE";
	static const std::string HWTRIGGER				= "HWTRIGGER";
	static const std::string HWTRIGGERPARAM			= "HWTRIGGERPARAM";
	static const std::string HWTRIGGERSOURCE		= "HWTRIGGERSOURCE";
	}

static const std::string BUFFER						= "IMACQUISITION.BUFFER";

static const std::string DO_PREVIEWS 				= "IMACQUISITION.DO_PREVIEWS";
static const std::string ANALYSISFILE				= "IMACQUISITION.ANALYSISFILE";
static const std::string LOGDIR 					= "IMACQUISITION.LOGDIR";
static const std::string IMDIR 						= "IMACQUISITION.IMDIR";
static const std::string IMDIRPREVIEW 				= "IMACQUISITION.IMDIRPREVIEW";
static const std::string EXCHANGEDIR 				= "IMACQUISITION.EXCHANGEDIR";
static const std::string EXCHANGEDIRPREVIEW         = "IMACQUISITION.EXCHANGEDIRPREVIEW";
}


