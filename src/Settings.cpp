#include "settings.h"


bool Settings::loadSettings(bool a_dumpParse)
{
	setFileName(FILE_NAME);
	return Json2Settings::Settings::loadSettings(a_dumpParse);
}


decltype(Settings::manageAmmo)		Settings::manageAmmo("manageAmmo", true, true);
decltype(Settings::manageHelmet)	Settings::manageHelmet("manageHelmet", true, true);
decltype(Settings::manageShield)	Settings::manageShield("manageShield", true, true);
