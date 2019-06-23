#include "settings.h"


bool Settings::loadSettings(bool a_dumpParse)
{
	return Json2Settings::Settings::loadSettings(FILE_NAME, false, a_dumpParse);
}


decltype(Settings::manageAmmo)		Settings::manageAmmo("manageAmmo", true, true);
decltype(Settings::manageHelmet)	Settings::manageHelmet("manageHelmet", true, true);
decltype(Settings::manageShield)	Settings::manageShield("manageShield", true, true);
