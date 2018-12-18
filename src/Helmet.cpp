#include "Helmet.h"

#include "skse64_common/Relocation.h"  // RelocPtr
#include "skse64_common/SafeWrite.h"  // SafeWrite64

#include <type_traits>  // typeid

#include "version.h"  // MAKE_STR

#include "RE/BaseExtraList.h"  // BaseExtraList
#include "RE/BSTEvent.h"  // EventResult, BSTEventSource
#include "RE/ButtonEvent.h"  // ButtonEvent
#include "RE/EquipManager.h"  // EquipManager
#include "RE/ExtraDataTypes.h"  // ExtraDataType
#include "RE/FormTypes.h"  // FormType
#include "RE/InventoryEntryData.h"  // InventoryEntryData
#include "RE/PlayerCharacter.h"  // PlayerCharacter
#include "RE/PlayerControls.h"  // PlayerControls::Data024
#include "RE/ReadyWeaponHandler.h"  // ReadyWeaponHandler
#include "RE/TESObjectARMO.h"  // TESObjectARMO


namespace Helmet
{
	Helmet::Helmet() :
		ISerializableForm()
	{}


	Helmet::~Helmet()
	{}


	const char* Helmet::ClassName() const
	{
		return MAKE_STR(Helmet);
	}


	UInt32 Helmet::ClassVersion() const
	{
		return kVersion;
	}


	UInt32 Helmet::ClassType() const
	{
		return 'HLMT';
	}


	bool Helmet::Save(json& a_save)
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


	bool Helmet::Load(json& a_load)
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


	RE::TESObjectARMO* Helmet::GetArmorForm()
	{
		if (GetLoadedFormID() == kInvalid) {
			return 0;
		} else {
			return RE::TESForm::LookupByID<RE::TESObjectARMO>(_loadedFormID);
		}
	}


	void HelmetTaskDelegate::Run()
	{
		if (_equip) {
			HelmetEquipVisitor visitor;
			VisitPlayerInventoryChanges(&visitor);
		} else {
			HelmetUnEquipVisitor visitor;
			VisitPlayerInventoryChanges(&visitor);
		}
	}


	void HelmetTaskDelegate::Dispose()
	{
		if (this) {
			delete this;
		}
	}


	bool HelmetTaskDelegate::HelmetEquipVisitor::Accept(RE::InventoryEntryData* a_entry)
	{
		typedef RE::BGSBipedObjectForm::BipedBodyTemplate::FirstPersonFlag FirstPersonFlag;

		if (a_entry->type && a_entry->type->formID == g_lastEquippedHelmet.GetLoadedFormID()) {
			RE::TESObjectARMO* armor = static_cast<RE::TESObjectARMO*>(a_entry->type);
			RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
			RE::BaseExtraList* xList = (a_entry->extraList && !a_entry->extraList->empty()) ? a_entry->extraList->front() : 0;
			equipManager->EquipItem(player, armor, xList, 1, armor->equipSlot, true, false, false, 0);
			return false;
		}
		return true;
	}


	bool HelmetTaskDelegate::HelmetUnEquipVisitor::Accept(RE::InventoryEntryData* a_entry)
	{
		typedef RE::BGSBipedObjectForm::BipedBodyTemplate::FirstPersonFlag FirstPersonFlag;

		if (a_entry->type && a_entry->type->Is(RE::FormType::Armor) && a_entry->extraList) {
			for (auto& xList : *a_entry->extraList) {
				if (xList->HasType(RE::ExtraDataType::kWorn)) {
					RE::TESObjectARMO* armor = static_cast<RE::TESObjectARMO*>(a_entry->type);
					if (armor->GetSlotMask() == FirstPersonFlag::kHead) {
						RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
						RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
						equipManager->UnEquipItem(player, armor, xList, 1, armor->equipSlot, true, false, true, false, 0);
						return false;
					}
				}
			}
		}
		return true;
	}


	void UnequipHelmet()
	{}


	class ReadyWeaponHandlerEx : public RE::ReadyWeaponHandler
	{
	public:
		typedef void _ProcessButton_t(RE::ReadyWeaponHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControls::Data024* a_arg2);
		static _ProcessButton_t* orig_ProcessButton;


		void Hook_ProcessButton(RE::ButtonEvent* a_event, RE::PlayerControls::Data024* a_arg2)
		{
			if (a_event->IsDown()) {
				RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
				TaskDelegate* dlgt = new HelmetTaskDelegate(!player->IsWeaponDrawn());
				g_task->AddTask(dlgt);
			}

			orig_ProcessButton(this, a_event, a_arg2);
		}


		static void InstallHooks()
		{
			constexpr uintptr_t READY_WEAPON_HANDLER_VTBL = 0x016892B8;
			RelocPtr<_ProcessButton_t*> vtbl_ProcessButton(READY_WEAPON_HANDLER_VTBL);
			orig_ProcessButton = *vtbl_ProcessButton;
			SafeWrite64(vtbl_ProcessButton.GetUIntPtr(), GetFnAddr(&Hook_ProcessButton));
			_DMESSAGE("[DEBUG] Installed hook for class (%s)", typeid(ReadyWeaponHandlerEx).name());
		}
	};


	ReadyWeaponHandlerEx::_ProcessButton_t* ReadyWeaponHandlerEx::orig_ProcessButton;


	void InstallHooks()
	{
		ReadyWeaponHandlerEx::InstallHooks();
	}


	void DelayedHelmetLocator::Run()
	{
		Visitor visitor(_formID);
		VisitPlayerInventoryChanges(&visitor);
	}


	void DelayedHelmetLocator::Dispose()
	{
		if (this) {
			delete this;
		}
	}


	bool DelayedHelmetLocator::Visitor::Accept(RE::InventoryEntryData* a_entry)
	{
		if (a_entry->type && a_entry->type->formID == _formID && a_entry->extraList) {
			for (auto& xList : *a_entry->extraList) {
				if (xList->HasType(RE::ExtraDataType::kWorn)) {
					g_lastEquippedHelmet.SetForm(_formID);
					return false;
				}
			}
		}
		return true;
	}


	RE::EventResult TESEquipEventHandler::ReceiveEvent(RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource)
	{
		using RE::EventResult;
		typedef RE::BGSBipedObjectForm::BipedBodyTemplate::FirstPersonFlag FirstPersonFlag;

		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		if (!a_event || !a_event->akSource || a_event->akSource->formID != player->formID) {
			return EventResult::kContinue;
		}

		RE::TESForm* form = RE::TESForm::LookupByID(a_event->formID);
		if (!form) {
			return EventResult::kContinue;
		}

		switch (form->formType) {
		case RE::FormType::Armor:
		{
			if (a_event->isEquipping) {
				RE::TESObjectARMO* armor = static_cast<RE::TESObjectARMO*>(form);
				if (armor->bipedBodyTemplate.firstPersonFlag == FirstPersonFlag::kHead) {
					DelayedHelmetLocator* dlgt = new DelayedHelmetLocator(form->formID);
					g_task->AddTask(dlgt);
				}
			}
			break;
		}
		}
		return EventResult::kContinue;
	}


	Helmet g_lastEquippedHelmet;
	TESEquipEventHandler g_equipEventSink;
}
