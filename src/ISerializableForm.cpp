#include "ISerializableForm.h"

#include "skse64/GameData.h"  // ModInfo

#include "RE/TESDataHandler.h"  // TESDataHandler


ISerializableForm::ISerializableForm() :
	_rawFormID(kInvalid),
	_loadedFormID(kInvalid),
	_pluginName(""),
	_isLightMod(false)
{}


ISerializableForm::~ISerializableForm()
{}


void ISerializableForm::Clear()
{
	_rawFormID = kInvalid;
	_loadedFormID = kInvalid;
	_pluginName = "";
	_isLightMod = false;
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
	const ModInfo* modInfo = 0;
	bool isLightMod = idx == 0xFE;
	if (isLightMod) {
		UInt16 lightIdx = a_formID << (1 * 8);
		lightIdx = lightIdx >> ((2 * 8) + 4);
		modInfo = dataHandler->LookupLoadedLightModByIndex(lightIdx);
		rawFormID = rawFormID << ((2 * 8) + 4);
		rawFormID = rawFormID >> ((2 * 8) + 4);
	} else {
		modInfo = dataHandler->LookupLoadedModByIndex(idx);
		rawFormID = rawFormID << (1 * 8);
		rawFormID = rawFormID >> (1 * 8);
	}

	if (modInfo) {
		_pluginName = modInfo->name;
		_rawFormID = rawFormID;
		_loadedFormID = a_formID;
		_isLightMod = isLightMod;
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
