
//
#include "ImgAcquisitionApp.h"
#include "settings/Settings.h"

int main(int argc, char *argv[])
{
	SettingsIAC::setConf("configImAcq.json");
	SettingsIAC *set = SettingsIAC::getInstance();

	ImgAcquisitionApp a(argc, argv);  //An instance is initialized

	return a.exec();
}
