#include "ISerializableForm.h"

#include "RE/Skyrim.h"
#include "SKSE/Interfaces.h"


ISerializableForm::ISerializableForm() :
	_formID(kInvalid)
{}


void ISerializableForm::Clear()
{
	_formID = kInvalid;
}


bool ISerializableForm::Save(SKSE::SerializationInterface* a_intfc, UInt32 a_type, UInt32 a_version)
{
	if (!a_intfc->OpenRecord(a_type, a_version)) {
		_ERROR("[ERROR] Failed to open serialization record!\n");
		return false;
	} else {
		return Save(a_intfc);
	}
}


bool ISerializableForm::Save(SKSE::SerializationInterface* a_intfc)
{
	a_intfc->WriteRecordData(&_formID, sizeof(_formID));

	return true;
}


bool ISerializableForm::Load(SKSE::SerializationInterface* a_intfc)
{
	a_intfc->ReadRecordData(&_formID, sizeof(_formID));
	if (!a_intfc->ResolveFormID(_formID, _formID)) {
		_ERROR("[ERROR] Failed to resolve formID");
		_formID = kInvalid;
		return false;
	}

	return true;
}


void ISerializableForm::SetForm(UInt32 a_formID)
{
	_formID = a_formID;
}


RE::TESForm* ISerializableForm::GetForm()
{
	return _formID == kInvalid ? 0 : RE::TESForm::LookupByID(_formID);
}


UInt32 ISerializableForm::GetFormID()
{
	return _formID;
}
