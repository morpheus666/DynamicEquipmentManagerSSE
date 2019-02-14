#include "Ammo.h"

#include "skse64/GameData.h"  // ModInfo

#include <exception>  // exception

#include "json.hpp"  // json
#include "version.h"  // MAKE_STR

#include "RE/BSTEvent.h"  // EventResult, BSTEventSource
#include "RE/EquipManager.h"  // EquipManager
#include "RE/ExtraDataTypes.h"  // ExtraDataType
#include "RE/FormTypes.h"  // FormType
#include "RE/InventoryEntryData.h"  // InventoryEntryData
#include "RE/InventoryMenu.h"  // InventoryMenu
#include "RE/PlayerCharacter.h"  // PlayerCharacter
#include "RE/MenuManager.h"  // MenuManager
#include "RE/TESAmmo.h"  // TESAmmo
#include "RE/TESDataHandler.h"  // TESDataHandler
#include "RE/TESEquipEvent.h"  // TESEquipEvent
#include "RE/TESForm.h"  // TESForm
#include "RE/TESObjectWEAP.h"  // TESObjectWEAP
#include "RE/UIStringHolder.h"  // UIStringHolder

#include "Ammo.h"  // g_lastEquippedAmmo
#include "Forms.h"  // Form


namespace Ammo
{
	Ammo::Ammo() :
		ISerializableForm()
	{}


	Ammo::~Ammo()
	{}


	const char* Ammo::ClassName() const
	{
		return MAKE_STR(Ammo);
	}


	RE::TESAmmo* Ammo::GetAmmoForm()
	{
		if (GetLoadedFormID() == kInvalid) {
			return 0;
		} else {
			return RE::TESForm::LookupByID<RE::TESAmmo>(_loadedFormID);
		}
	}


	void DelayedWeaponTaskDelegate::Run()
	{
		typedef RE::TESObjectWEAP::Data::AnimationType AnimationType;

		if (g_equippedWeaponFormID != kInvalid) {
			RE::TESObjectWEAP* weap = RE::TESForm::LookupByID<RE::TESObjectWEAP>(g_equippedWeaponFormID);

			if ((!weap->IsBow() && !weap->IsCrossbow()) || weap->IsBound()) {
				g_equippedWeaponFormID = kInvalid;
				return;
			}

			Visitor visitor;
			VisitPlayerInventoryChanges(&visitor);

			RE::TESAmmo* ammo = g_lastEquippedAmmo.GetAmmoForm();
			if (ammo) {
				RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
				RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
				equipManager->EquipItem(player, ammo, 0, visitor.Count(), 0, true, false, false);

				RE::MenuManager* mm = RE::MenuManager::GetSingleton();
				RE::UIStringHolder* uiStrHolder = RE::UIStringHolder::GetSingleton();
				RE::InventoryMenu* invMenu = mm->GetMenu<RE::InventoryMenu>(uiStrHolder->inventoryMenu);
				if (invMenu && invMenu->inventoryData) {
					invMenu->inventoryData->Update(player);
				}
			}

			g_equippedWeaponFormID = kInvalid;
		}
	}


	void DelayedWeaponTaskDelegate::Dispose()
	{
		delete this;
	}


	bool DelayedWeaponTaskDelegate::Visitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		if (a_entry->type->IsAmmo()) {
			if (a_entry->type->formID == g_lastEquippedAmmo.GetLoadedFormID()) {
				_count = a_count;
				return false;
			}
		}
		return true;
	}


	bool DelayedAmmoTaskDelegate::Visitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		if (a_entry->type->formID == g_equippedAmmoFormID && a_entry->extraList) {
			for (auto& xList : *a_entry->extraList) {
				if (xList->HasType(RE::ExtraDataType::kWorn) || xList->HasType(RE::ExtraDataType::kWornLeft)) {
					RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
					RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
					equipManager->UnEquipItem(player, a_entry->type, xList, a_count, 0, true, false);

					RE::MenuManager* mm = RE::MenuManager::GetSingleton();
					RE::UIStringHolder* uiStrHolder = RE::UIStringHolder::GetSingleton();
					RE::InventoryMenu* invMenu = mm->GetMenu<RE::InventoryMenu>(uiStrHolder->inventoryMenu);
					if (invMenu && invMenu->inventoryData) {
						invMenu->inventoryData->Update(player);
					}

					return false;
				}
			}
		}
		return true;
	}


	void DelayedAmmoTaskDelegate::Run()
	{
		if (g_equippedAmmoFormID != kInvalid) {
			RE::TESAmmo* ammo = RE::TESForm::LookupByID<RE::TESAmmo>(g_equippedAmmoFormID);
			if (!ammo->HasKeyword(WeapTypeBoundArrow)) {
				if (g_equippedWeaponFormID == kInvalid) {
					g_lastEquippedAmmo.SetForm(g_equippedAmmoFormID);
				} else {
					// Ammo was force equipped
					Visitor visitor;
					VisitPlayerInventoryChanges(&visitor);
				}
			}
			g_equippedAmmoFormID = kInvalid;
		}
	}


	void DelayedAmmoTaskDelegate::Dispose()
	{
		delete this;
	}


	TESEquipEventHandler::TESEquipEventHandler()
	{}


	TESEquipEventHandler::~TESEquipEventHandler()
	{}


	RE::EventResult TESEquipEventHandler::ReceiveEvent(RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource)
	{
		using RE::EventResult;

		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		if (!a_event || !a_event->akSource || a_event->akSource->formID != player->formID) {
			return EventResult::kContinue;
		}

		RE::TESForm* form = RE::TESForm::LookupByID(a_event->formID);
		if (!form) {
			return EventResult::kContinue;
		}

		switch (form->formType) {
		case RE::FormType::Weapon:
			{
				if (a_event->isEquipping) {
					g_equippedWeaponFormID = form->formID;
					DelayedWeaponTaskDelegate* dlgt = new DelayedWeaponTaskDelegate();
					g_task->AddTask(dlgt);
				} else {
					Visitor visitor;
					VisitPlayerInventoryChanges(&visitor);
				}
			}
			break;
		case RE::FormType::Ammo:
			{
				if (a_event->isEquipping) {
					g_equippedAmmoFormID = form->formID;
					DelayedAmmoTaskDelegate* dlgt = new DelayedAmmoTaskDelegate();
					g_task->AddTask(dlgt);
				}
			}
			break;
		}
		return EventResult::kContinue;
	}


	TESEquipEventHandler* TESEquipEventHandler::GetSingleton()
	{
		if (!_singleton) {
			_singleton = new TESEquipEventHandler();
		}
		return _singleton;
	}


	void TESEquipEventHandler::Free()
	{
		delete _singleton;
		_singleton = 0;
	}


	TESEquipEventHandler* TESEquipEventHandler::_singleton = 0;


	bool TESEquipEventHandler::Visitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		if (a_entry->type->formID == g_lastEquippedAmmo.GetLoadedFormID() && a_entry->extraList) {
			RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
			RE::BaseExtraList* xList = a_entry->extraList->empty() ? 0 : a_entry->extraList->front();
			equipManager->UnEquipItem(player, a_entry->type, xList, a_count, 0, true, false);
			return false;
		}
		return true;
	}


	Ammo g_lastEquippedAmmo;
}
