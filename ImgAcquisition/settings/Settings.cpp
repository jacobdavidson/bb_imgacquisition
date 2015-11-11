#include "Settings.h"

#include "boost/program_options.hpp"

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

	    pt.put(IMACQUISITION::VIDEO_WIDTH, 4000);
	    pt.put(IMACQUISITION::VIDEO_HEIGHT, 3000);
	    pt.put(IMACQUISITION::BITRATE, 1000000);
	    pt.put(IMACQUISITION::RCMODE, 6);
	    pt.put(IMACQUISITION::QP, 2);
	    pt.put(IMACQUISITION::FRAMESPERVIDEO, 500);
	    pt.put(IMACQUISITION::BUFFERSIZE, 20);
	    pt.put(IMACQUISITION::FPS, 3);
	    pt.put(IMACQUISITION::PRESET, 20);
	    pt.put(IMACQUISITION::LOGDIR, "./log/Cam_%d/");
	    pt.put(IMACQUISITION::IMDIR, "./out/Cam_%u/Cam_%u_%s.mp4");


    return pt;
}
