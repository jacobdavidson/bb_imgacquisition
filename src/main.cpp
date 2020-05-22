// SPDX-License-Identifier: GPL-3.0-or-later

#include "ImgAcquisitionApp.h"
#include "settings/Settings.h"

int main(int argc, char* argv[])
{
    SettingsIAC::setConf("configImAcq.json");
    SettingsIAC::getInstance();

    ImgAcquisitionApp app(argc, argv);

    return app.exec();
}
