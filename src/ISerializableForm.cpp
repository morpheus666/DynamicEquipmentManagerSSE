#include "ISerializableForm.h"

#include <exception>  // exception

#include "version.h"  // MAKE_STR

#include "RE/TESDataHandler.h"  // TESDataHandler
#include "RE/TESFile.h"  // TESFile


ISerializableForm::ISerializableForm() :
	_rawFormID(kInvalid),
	_loadedFormID(kInvalid),
	_pluginName(""),
	_isLightMod(false),
	_isGeneratedID(false)
{}


ISerializableForm::~ISerializableForm()
{}


void ISerializableForm::Clear()
{
	_rawFormID = kInvalid;
	_loadedFormID = kInvalid;
	_pluginName = "";
	_isLightMod = false;
	_isGeneratedID = false;
}


bool ISerializableForm::Save(json& a_save)
{
	try {
		a_save = {
			{ MAKE_STR(_rawFormID), _rawFormID },
			{ MAKE_STR(_pluginName), _pluginName },
			{ MAKE_STR(_isLightMod), _isLightMod },
			{ MAKE_STR(_isGeneratedID), _isGeneratedID }
		};
	} catch (std::exception& e) {
		_ERROR("[ERROR] %s", e.what());
		return false;
	}

	return true;
}


bool ISerializableForm::Load(json& a_load)
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

		if (!loadJsonObj(a_load, MAKE_STR(_isGeneratedID), _isGeneratedID)) {
			return false;
		}
	} catch (std::exception& e) {
		_ERROR("[ERROR] %s", e.what());
		return false;
	}

	return true;
}


#pragma warning (push)
#pragma warning (disable : 4333)  // '>>': right shift by too large amount, data loss
void ISerializableForm::SetForm(UInt32 a_formID)
{
	if (a_formID == _loadedFormID) {
		return;
	}

	RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
	UInt32 rawFormID = a_formID;
	UInt8 idx = a_formID >> (3 * 8);
	const RE::TESFile* modInfo = 0;
	bool isLightMod = idx == 0xFE;
	bool isGeneratedID = idx == 0xFF;
	if (isLightMod) {
		UInt16 lightIdx = a_formID << (1 * 8);
		lightIdx = lightIdx >> ((2 * 8) + 4);
		modInfo = dataHandler->LookupLoadedLightModByIndex(lightIdx);
		rawFormID = rawFormID << ((2 * 8) + 4);
		rawFormID = rawFormID >> ((2 * 8) + 4);
	} else {
		if (!isGeneratedID) {
			modInfo = dataHandler->LookupLoadedModByIndex(idx);
			rawFormID = rawFormID << (1 * 8);
			rawFormID = rawFormID >> (1 * 8);
		}
	}

	if (isGeneratedID || modInfo) {
		_pluginName = modInfo ? modInfo->name : "Skyrim.esm";
		_rawFormID = rawFormID;
		_loadedFormID = a_formID;
		_isLightMod = isLightMod;
		_isGeneratedID = isGeneratedID;
	}
}
#pragma warning (pop)


UInt32 ISerializableForm::GetLoadedFormID()
{
	if (_rawFormID == kInvalid) {
		return kInvalid;
	}

	if (_loadedFormID == kInvalid) {
		RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
		const RE::TESFile* modInfo = 0;
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
			if (_isGeneratedID) {
				_loadedFormID = _rawFormID;
				_loadedFormID += 0xFF << (3 * 8);
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
	}

	return _loadedFormID;
}
