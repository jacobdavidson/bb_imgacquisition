#include "Settings.h"

#include "boost/program_options.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <sstream>

const boost::property_tree::ptree SettingsIAC::getDefaultParams() {

	boost::property_tree::ptree pt;
	std::string app = SettingsIAC::setConf("");

	for (int i=0; i<4; i++){

		boost::property_tree::ptree hd;
		hd.put(IMACQUISITION::BUFFERCONF::CAMID,	 		i		);
		hd.put(IMACQUISITION::BUFFERCONF::ISPREVIEW, 		0		);
		hd.put(IMACQUISITION::BUFFERCONF::SERIAL,	 		0		);
		hd.put(IMACQUISITION::BUFFERCONF::ENABLED,	 		1		);
		hd.put(IMACQUISITION::BUFFERCONF::VIDEO_WIDTH, 		4000	);
		hd.put(IMACQUISITION::BUFFERCONF::VIDEO_HEIGHT, 	3000	);
		hd.put(IMACQUISITION::BUFFERCONF::BITRATE, 			1000000	);
		hd.put(IMACQUISITION::BUFFERCONF::RCMODE, 			0		);
		hd.put(IMACQUISITION::BUFFERCONF::QP, 				25		);
		hd.put(IMACQUISITION::BUFFERCONF::FRAMESPERVIDEO, 	500		);
		hd.put(IMACQUISITION::BUFFERCONF::FPS, 				3		);
		hd.put(IMACQUISITION::BUFFERCONF::PRESET, 			2		);
		hd.put(IMACQUISITION::BUFFERCONF::OFFSETX, 			0		);
		hd.put(IMACQUISITION::BUFFERCONF::OFFSETY, 			0		);
		hd.put(IMACQUISITION::BUFFERCONF::HWBUFSIZE,		0		);
		hd.put(IMACQUISITION::BUFFERCONF::BRIGHTNESSONOFF, 	1		);
		hd.put(IMACQUISITION::BUFFERCONF::BRIGHTNESSAUTO,	0		);
		hd.put(IMACQUISITION::BUFFERCONF::BRIGHTNESS,		0		);
		hd.put(IMACQUISITION::BUFFERCONF::EXPOSUREONOFF,	0		);
		hd.put(IMACQUISITION::BUFFERCONF::EXPOSUREAUTO,		0		);
		hd.put(IMACQUISITION::BUFFERCONF::SHUTTER,			40		);
		hd.put(IMACQUISITION::BUFFERCONF::SHUTTERONOFF,		1		);
		hd.put(IMACQUISITION::BUFFERCONF::SHUTTERAUTO,		0		);
		hd.put(IMACQUISITION::BUFFERCONF::GAIN,				0		);
		hd.put(IMACQUISITION::BUFFERCONF::GAINONOFF,		1		);
		hd.put(IMACQUISITION::BUFFERCONF::GAINAUTO,			0		);
		hd.put(IMACQUISITION::BUFFERCONF::WHITEBALANCE,		0		);
		hd.put(IMACQUISITION::BUFFERCONF::HWTRIGGER,		0		);
		hd.put(IMACQUISITION::BUFFERCONF::HWTRIGGERPARAM,	0		);
		hd.put(IMACQUISITION::BUFFERCONF::HWTRIGGERSOURCE,	0		);

		boost::property_tree::ptree ld;
		ld.put(IMACQUISITION::BUFFERCONF::CAMID,	 		i		);
		ld.put(IMACQUISITION::BUFFERCONF::ISPREVIEW, 		1		);
		ld.put(IMACQUISITION::BUFFERCONF::SERIAL,	 		0		);
		ld.put(IMACQUISITION::BUFFERCONF::ENABLED,	 		1		);
		ld.put(IMACQUISITION::BUFFERCONF::VIDEO_WIDTH, 		2000	);
		ld.put(IMACQUISITION::BUFFERCONF::VIDEO_HEIGHT, 	1500	);
		ld.put(IMACQUISITION::BUFFERCONF::BITRATE, 			1000000	);
		ld.put(IMACQUISITION::BUFFERCONF::RCMODE, 			0		);
		ld.put(IMACQUISITION::BUFFERCONF::QP, 				35		);
		ld.put(IMACQUISITION::BUFFERCONF::FRAMESPERVIDEO, 	500		);
		ld.put(IMACQUISITION::BUFFERCONF::FPS, 				3		);
		ld.put(IMACQUISITION::BUFFERCONF::PRESET, 			2		);
	    pt.add_child(IMACQUISITION::BUFFER, hd);
	    pt.add_child(IMACQUISITION::BUFFER, ld);
	}

	pt.put(IMACQUISITION::DO_PREVIEWS, 			1		);
	pt.put(IMACQUISITION::ANALYSISFILE,			"./analysis.txt");
	pt.put(IMACQUISITION::LOGDIR, 				"./log/Cam_%d/");
	pt.put(IMACQUISITION::IMDIR, 				"./tmp/Cam_%u/Cam_%u_%s--%s");
	pt.put(IMACQUISITION::EXCHANGEDIR, 			"./out/Cam_%u/");
	pt.put(IMACQUISITION::IMDIRPREVIEW, 		"./tmpPrev/Cam_%u/Cam_%u_%s--%s");
    pt.put(IMACQUISITION::EXCHANGEDIRPREVIEW,   "./outPrev/Cam_%u/");
    pt.put(IMACQUISITION::SLACKPOST,            "/home/moenck/slackpost.sh bb_notification On Emma: ");
    pt.put(IMACQUISITION::POSTLEVEL1,            "@moenck ");
    pt.put(IMACQUISITION::POSTLEVEL2,            "@channel ");
    pt.put(IMACQUISITION::CAMCOUNT,             2);


	return pt;
}


EncoderQualityConfig SettingsIAC::setFromNode(boost::property_tree::ptree node){
	EncoderQualityConfig cfg;

	cfg.camid 				= node.get<int>(IMACQUISITION::BUFFERCONF::CAMID);
	cfg.isPreview 			= node.get<int>(IMACQUISITION::BUFFERCONF::ISPREVIEW);
	cfg.serial	 			= node.get<int>(IMACQUISITION::BUFFERCONF::SERIAL);
	cfg.enabled 			= node.get<int>(IMACQUISITION::BUFFERCONF::ENABLED);
	cfg.rcmode 				= node.get<int>(IMACQUISITION::BUFFERCONF::RCMODE);
	cfg.preset 				= node.get<int>(IMACQUISITION::BUFFERCONF::PRESET);
	cfg.qp 					= node.get<int>(IMACQUISITION::BUFFERCONF::QP);
	cfg.bitrate 			= node.get<int>(IMACQUISITION::BUFFERCONF::BITRATE);
	cfg.totalFrames			= node.get<int>(IMACQUISITION::BUFFERCONF::FRAMESPERVIDEO);
	cfg.width 				= node.get<int>(IMACQUISITION::BUFFERCONF::VIDEO_WIDTH);
	cfg.height 				= node.get<int>(IMACQUISITION::BUFFERCONF::VIDEO_HEIGHT);
	cfg.fps 				= node.get<int>(IMACQUISITION::BUFFERCONF::FPS);

	if (cfg.isPreview==0){
		cfg.offsetx 		= node.get<int>(IMACQUISITION::BUFFERCONF::OFFSETX);
		cfg.offsety 		= node.get<int>(IMACQUISITION::BUFFERCONF::OFFSETY);
		cfg.hwbuffersize	= node.get<int>(IMACQUISITION::BUFFERCONF::HWBUFSIZE);
		cfg.brightness 		= node.get<int>(IMACQUISITION::BUFFERCONF::BRIGHTNESS);
		cfg.brightnessonoff	= node.get<int>(IMACQUISITION::BUFFERCONF::BRIGHTNESSONOFF);
		cfg.brightnessauto	= node.get<int>(IMACQUISITION::BUFFERCONF::BRIGHTNESSAUTO);
		cfg.exposureonoff	= node.get<int>(IMACQUISITION::BUFFERCONF::EXPOSUREONOFF);
		cfg.exposureauto	= node.get<int>(IMACQUISITION::BUFFERCONF::EXPOSUREAUTO);
		cfg.shutter 		= node.get<int>(IMACQUISITION::BUFFERCONF::SHUTTER);
		cfg.shutteronoff	= node.get<int>(IMACQUISITION::BUFFERCONF::SHUTTERONOFF);
		cfg.shutterauto		= node.get<int>(IMACQUISITION::BUFFERCONF::SHUTTERAUTO);
		cfg.gain		 	= node.get<int>(IMACQUISITION::BUFFERCONF::GAIN);
		cfg.gainonoff	 	= node.get<int>(IMACQUISITION::BUFFERCONF::GAINONOFF);
		cfg.gainauto	 	= node.get<int>(IMACQUISITION::BUFFERCONF::GAINAUTO);
		cfg.whitebalance 	= node.get<int>(IMACQUISITION::BUFFERCONF::WHITEBALANCE);
		cfg.hwtrigger 		= node.get<int>(IMACQUISITION::BUFFERCONF::HWTRIGGER);
		cfg.hwtriggerparam	= node.get<int>(IMACQUISITION::BUFFERCONF::HWTRIGGER);
		cfg.hwtriggersrc	= node.get<int>(IMACQUISITION::BUFFERCONF::HWTRIGGER);
	}
	return cfg;
}

EncoderQualityConfig SettingsIAC::getBufferConf(int camid, int preview){
	EncoderQualityConfig 	cfg;
	std::stringstream 		cid, prev;

	cfg.camid 	= -1;
	cid 		<< camid;
	prev 		<< preview;

	//Find the subtree having the right CAMID and ISPREVIEW values
	BOOST_FOREACH(boost::property_tree::ptree::value_type &v,
			_ptree.get_child("IMACQUISITION")){
		if(v.first =="BUFFER"){
			int hit = 0;
			BOOST_FOREACH(boost::property_tree::ptree::value_type &w, v.second){
				std::string snd = w.second.data();
				if (w.first == "CAMID" && snd == cid.str()) hit ++;
				if (w.first == "ISPREVIEW" && snd == prev.str()) hit ++;

				if (hit==2) {
					return setFromNode(v.second);
				}
			}
		}

	}
	return cfg;
}


