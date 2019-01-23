#pragma once

#include <string>  // string

#include "RE/BGSKeyword.h"  // BGSKeyword
#include "RE/TESDataHandler.h"  // TESDataHandler
#include "RE/TESFile.h"  // TESFile
#include "RE/TESForm.h"  // LookupByID


enum SkyrimFormID : UInt32
{
	kSkyrimFormID_WeapTypeBoundArrow = 0x10D501
};


template <typename T>
class Form
{
public:
	enum
	{
		kInvalid = 0xFFFFFFFF
	};


	Form(UInt32 a_rawFormID, std::string a_pluginName) :
		_rawFormID(a_rawFormID),
		_loadedFormID(kInvalid),
		_pluginName(a_pluginName)
	{}


	operator T*()
	{
		if (_rawFormID == kInvalid) {
			return 0;
		}

		if (_loadedFormID == kInvalid) {
			RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
			const RE::TESFile* modInfo = dataHandler->LookupLoadedModByName(_pluginName.c_str());
			if (modInfo) {
				_loadedFormID = _rawFormID + (modInfo->modIndex << (3 * 8));
			} else {
				_rawFormID = kInvalid;
				return 0;
			}
		}

		return RE::TESForm::LookupByID<T>(_loadedFormID);
	}

private:
	UInt32		_rawFormID;
	UInt32		_loadedFormID;
	std::string	_pluginName;
};


extern Form<RE::BGSKeyword> WeapTypeBoundArrow;
