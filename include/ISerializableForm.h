#pragma once

#undef SetForm

#include "RE/Memory.h"  // TES_HEAP_REDEFINE_NEW

#include "json.hpp"  // json


enum
{
	kInvalid = 0xFFFFFFFF
};


// Needs to handle generated formIDs
class ISerializableForm
{
private:
	using json = nlohmann::json;

public:
	ISerializableForm();
	ISerializableForm(ISerializableForm& a_ISerializableForm) = default;
	ISerializableForm(ISerializableForm&& a_ISerializableForm) = default;

	virtual ~ISerializableForm();

	TES_HEAP_REDEFINE_NEW();

	virtual const char*		ClassName() const = 0;
	virtual UInt32			ClassVersion() const = 0;
	virtual UInt32			ClassType() const = 0;
	void					Clear();
	virtual bool			Save(json& a_save) = 0;
	virtual bool			Load(json& a_load) = 0;
	void					SetForm(UInt32 a_formID);
	constexpr UInt32		GetRawFormID() const { return _rawFormID; }
	UInt32					GetLoadedFormID();

protected:
	UInt32		_rawFormID;
	UInt32		_loadedFormID;
	std::string	_pluginName;
	bool		_isLightMod;
};


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
