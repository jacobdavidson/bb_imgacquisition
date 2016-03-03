#include "Settings.h"

#include "boost/program_options.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <sstream>

int setCLIoptions(int argc, char **argv){
	namespace po = boost::program_options;
	namespace po_style = boost::program_options::command_line_style;
	SettingsIAC *set = SettingsIAC::getInstance();

	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
	    						("help,h", 		"Produce help message")
	    						("op1", 		"1")
	    						("op2", 		po::value<int>(),"2")
	    						("op3",			po::value<std::string>(),"3")
	    						;

	po::variables_map vm;
	try{
		po::store(po::command_line_parser(argc, argv).options(desc)
				.style(	po_style::unix_style
						| po_style::case_insensitive
						| boost::program_options::command_line_style::allow_long_disguise
				).run(), vm);
		po::notify(vm);
	}catch( boost::program_options::unknown_option & e )
	{
		std::cerr << e.what() <<std::endl;
		std::cout << desc << std::endl;
		exit(0);
	}catch( ... )
	{
		std::cerr <<"Unrecognized exception. Did you forget to set a value?\n" ;
		exit(0);
	}
	if (vm.count("help") || argc<1) {
		std::cout << desc << std::endl;
		return 1;
	}

	boost::property_tree::ptree *pt = &(set->_ptree);
	//if (vm.count("ebr")) pt->put(ENCODERPARAM::ENC_BITRATE, vm["ebr"].as<int>());
	//if (vm.count("i"))pt->put(TAGSTRINGPARAM::INFOLDER, vm["i"].as<std::string>());

	return 0;
}

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
	pt.put(IMACQUISITION::LOGDIR, 				"./log/Cam_%d/");
	pt.put(IMACQUISITION::IMDIR, 				"./tmp/Cam_%u/Cam_%u_%s_TO_Cam_%u_%s");
	pt.put(IMACQUISITION::EXCHANGEDIR, 			"./out/Cam_%u/");
	pt.put(IMACQUISITION::IMDIRPREVIEW, 		"./tmpPrev/Cam_%u/Cam_%u_%s_TO_Cam_%u_%s");
	pt.put(IMACQUISITION::EXCHANGEDIRPREVIEW, 	"./outPrev/Cam_%u/");


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


