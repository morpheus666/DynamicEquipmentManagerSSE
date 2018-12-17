#pragma once

#include <string>  // string

#include "json.hpp"  // json
#include "version.h"  // MAKE_STR

#include "RE/FormTypes.h"  // TESAmmo
#include "RE/Memory.h"  // TES_HEAP_REDEFINE_NEW


class Ammo
{
private:
	using json = nlohmann::json;

public:
	explicit Ammo();
	Ammo(Ammo& a_ammo) = default;
	Ammo(Ammo&& a_ammo) = default;

	TES_HEAP_REDEFINE_NEW();

	constexpr const char*	ClassName() const { return MAKE_STR(Ammo); }
	constexpr UInt32		ClassVersion() const { return kVersion; }
	constexpr UInt32		ClassType() const { return 'AMMO'; }
	void					Clear();
	bool					Save(json& a_save);
	bool					Load(json& a_load);
	constexpr void			SetRawFormID(UInt32 a_formID) { _rawFormID = a_formID; _loadedFormID = kInvalid; }
	constexpr UInt32		GetRawFormID() const { return _rawFormID; }
	UInt32					GetLoadedFormID();
	void					SetPluginName(const char* a_pluginName);
	RE::TESAmmo*			GetAmmoForm();

private:
	enum
	{
		kInvalid = 0xFFFFFFFF,
		kVersion = 1
	};


	UInt32		_rawFormID;
	UInt32		_loadedFormID;
	std::string	_pluginName;
	bool		_isLightMod;
};


extern Ammo g_lastEquippedAmmo;
