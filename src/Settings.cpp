#include "settings.h"


bool Settings::loadSettings(bool a_dumpParse)
{
	setFileName(FILE_NAME);
	return Json2Settings::Settings::loadSettings(a_dumpParse);
}


bSetting	Settings::manageAmmo("manageAmmo", true, true);
bSetting	Settings::manageHelmet("manageHelmet", true, true);
bSetting	Settings::manageShield("manageShield", true, true);
