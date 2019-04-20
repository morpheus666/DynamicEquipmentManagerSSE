#pragma once

#undef SetForm
#undef GetForm

#include "RE/Skyrim.h"
#include "SKSE/Interfaces.h"


enum : UInt32
{
	kInvalid = static_cast<UInt32>(-1)
};


class ISerializableForm
{
public:
	ISerializableForm();
	ISerializableForm(ISerializableForm&) = default;
	ISerializableForm(ISerializableForm&&) = default;
	~ISerializableForm() = default;

	ISerializableForm& operator=(const ISerializableForm&) = default;
	ISerializableForm& operator=(ISerializableForm&&) = default;

	void Clear();
	bool Save(SKSE::SerializationInterface* a_intfc, UInt32 a_type, UInt32 a_version);
	bool Save(SKSE::SerializationInterface* a_intfc);
	bool Load(SKSE::SerializationInterface* a_intfc);
	void SetForm(UInt32 a_formID);
	RE::TESForm* GetForm();
	UInt32 GetFormID();

protected:
	UInt32 _formID;
};
