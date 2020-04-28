// SPDX-License-Identifier: GPL-3.0-or-later

#include "ImgAcquisitionApp.h"
#include "settings/Settings.h"
#include "settings/utility.h"

int main(int argc, char* argv[])
{
    SettingsIAC::setConf("configImAcq.json");
    SettingsIAC::getInstance();

    ImgAcquisitionApp a(argc, argv); // An instance is initialized

    return a.exec();
}
