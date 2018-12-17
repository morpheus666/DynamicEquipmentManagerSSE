#include "Ammo.h"

#include "skse64/GameData.h"  // ModInfo

#include <exception>  // exception

#include "json.hpp"  // json
#include "version.h"  // MAKE_STR

#include "RE/TESAmmo.h"  // TESAmmo
#include "RE/TESDataHandler.h"  // TESDataHandler
#include "RE/TESForm.h"  // TESForm


Ammo::Ammo() :
	_rawFormID(kInvalid),
	_loadedFormID(kInvalid),
	_pluginName(""),
	_isLightMod(false)
{}


void Ammo::Clear()
{
	_rawFormID = kInvalid;
	_loadedFormID = kInvalid;
	_pluginName = "";
	_isLightMod = false;
}


bool Ammo::Save(json& a_save)
{
	try {
		a_save = {
			{MAKE_STR(_rawFormID), _rawFormID},
			{MAKE_STR(_pluginName), _pluginName},
			{MAKE_STR(_isLightMod), _isLightMod}
		};
	} catch (std::exception& e) {
		_ERROR("[ERROR] %s", e.what());
		return false;
	}

	return true;
}


template <typename T>
bool loadJsonObj(nlohmann::json& a_load, const char* a_name, T& a_val)
{
	auto& it = a_load.find(a_name);
	if (it != a_load.end()) {
		a_val = it.value();
		return true;
	} else {
		_ERROR("[ERROR] Could not find (%s) in serialized save data!\n", a_name);
		return false;
	}
}


bool Ammo::Load(json& a_load)
{
	try {
		if (!loadJsonObj(a_load, MAKE_STR(_rawFormID), _rawFormID)) {
			return false;
		}

		_loadedFormID = kInvalid;

		if (!loadJsonObj(a_load, MAKE_STR(_pluginName), _pluginName)) {
			return false;
		}

		if (!loadJsonObj(a_load, MAKE_STR(_isLightMod), _isLightMod)) {
			return false;
		}
	} catch (std::exception& e) {
		_ERROR("[ERROR] %s", e.what());
		return false;
	}

	return true;
}


UInt32 Ammo::GetLoadedFormID()
{
	if (_rawFormID == kInvalid) {
		return kInvalid;
	}

	if (_loadedFormID == kInvalid) {
		RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
		const ModInfo* modInfo = 0;
		if (_isLightMod) {
			modInfo = dataHandler->LookupLoadedLightModByName(_pluginName.c_str());
			if (!modInfo) {
				_rawFormID = kInvalid;
				return kInvalid;
			}
			_loadedFormID = _rawFormID;
			_loadedFormID += modInfo->lightIndex << ((1 * 8) + 4);
			_loadedFormID += 0xFE << (3 * 8);
		} else {
			modInfo = dataHandler->LookupLoadedModByName(_pluginName.c_str());
			if (!modInfo) {
				_rawFormID = kInvalid;
				return kInvalid;
			}
			_loadedFormID = _rawFormID;
			_loadedFormID += modInfo->modIndex << (3 * 8);
		}
	}

	return _loadedFormID;
}


void Ammo::SetPluginName(const char* a_pluginName)
{
	_pluginName = a_pluginName;
}


RE::TESAmmo* Ammo::GetAmmoForm()
{
	if (GetLoadedFormID() == kInvalid) {
		return 0;
	} else {
		return RE::TESForm::LookupByID<RE::TESAmmo>(_loadedFormID);
	}
}


Ammo g_lastEquippedAmmo;
