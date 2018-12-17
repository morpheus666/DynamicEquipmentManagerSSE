#include "Events.h"

#include "skse64/GameData.h"  // ModInfo
#include "skse64/PluginAPI.h"  // SKSETaskInterface

#include "RE/BSTEvent.h"  // EventResult, BSTEventSource
#include "RE/EquipManager.h"  // EquipManager
#include "RE/ExtraDataTypes.h"  // ExtraDataType
#include "RE/FormTypes.h"  // FormType
#include "RE/InventoryChanges.h"  // InventoryChanges
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


void VisitPlayerInventoryChanges(InventoryChangesVisitor* a_visitor)
{
	RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
	RE::InventoryChanges* changes = player->GetInventoryChanges();
	if (changes) {
		for (auto& entry : *changes->entryList) {
			if (!a_visitor->Accept(entry)) {
				break;
			}
		}
	}
}


void DelayedWeaponTaskDelegate::Run()
{
	typedef RE::TESObjectWEAP::Data::AnimationType AnimationType;

	if (g_equippedWeaponFormID != kInvalid) {
		RE::TESObjectWEAP* weap = RE::TESForm::LookupByID<RE::TESObjectWEAP>(g_equippedWeaponFormID);

		if (weap->data.animationType != AnimationType::kBow && weap->data.animationType != AnimationType::kCrossbow) {
			g_equippedWeaponFormID = kInvalid;
			return;
		}

		Visitor visitor;
		VisitPlayerInventoryChanges(&visitor);

		RE::TESAmmo* ammo = g_lastEquippedAmmo.GetAmmoForm();
		if (ammo) {
			RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
			equipManager->EquipItem(player, ammo, visitor.ExtraList(), visitor.Count(), 0, true, false, false, 0);

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
	if (this) {
		delete this;
	}
}


bool DelayedWeaponTaskDelegate::Visitor::Accept(RE::InventoryEntryData* a_entry)
{
	if (a_entry->type && a_entry->type->IsAmmo()) {
		if (a_entry->type->formID == g_lastEquippedAmmo.GetLoadedFormID()) {
			_extraList = a_entry->extraList ? a_entry->extraList->front() : 0;
			_count = a_entry->countDelta;
			return false;
		}
	}
	return true;
}


void DelayedAmmoTaskDelegate::Run()
{
	if (g_equippedAmmoFormID != kInvalid) {
		if (g_equippedWeaponFormID == kInvalid) {
			RE::TESAmmo* ammo = RE::TESForm::LookupByID<RE::TESAmmo>(g_equippedAmmoFormID);

			if (ammo->formID == g_lastEquippedAmmo.GetLoadedFormID()) {
				g_equippedAmmoFormID = kInvalid;
				return;
			}

			RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
			UInt32 rawFormID = ammo->formID;
			UInt8 idx = ammo->formID >> (3 * 8);
			const ModInfo* modInfo = 0;
			if (idx != 0xFE) {
				modInfo = dataHandler->LookupLoadedModByIndex(idx);
				rawFormID = rawFormID << (1 * 8);
				rawFormID = rawFormID >> (1 * 8);
			} else {
				UInt16 lightIdx = (ammo->formID << (1 * 8)) >> ((2 * 8) + 4);
				modInfo = dataHandler->LookupLoadedLightModByIndex(lightIdx);
				rawFormID = rawFormID << ((2 * 8) + 4);
			}

			if (!modInfo) {
				_ERROR("[ERROR] No mod info found for the given ammo form (name == %s) (formID == %u)", ammo->name, ammo->formID);
				g_equippedAmmoFormID = kInvalid;
				return;
			}

			g_lastEquippedAmmo.SetPluginName(modInfo->name);
			g_lastEquippedAmmo.SetRawFormID(rawFormID);
		} else {
			// Ammo was force equipped
			Visitor visitor;
			VisitPlayerInventoryChanges(&visitor);
		}
		g_equippedAmmoFormID = kInvalid;
	}
}


void DelayedAmmoTaskDelegate::Dispose()
{
	if (this) {
		delete this;
	}
}


bool DelayedAmmoTaskDelegate::Visitor::Accept(RE::InventoryEntryData* a_entry)
{
	if (a_entry->type && a_entry->type->formID == g_equippedAmmoFormID && a_entry->extraList) {
		for (auto& xList : *a_entry->extraList) {
			if (xList->HasType(RE::ExtraDataType::kWorn) || xList->HasType(RE::ExtraDataType::kWornLeft)) {
				RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
				RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
				equipManager->UnEquipItem(player, a_entry->type, xList, a_entry->countDelta, 0, true, false, true, false, 0);
				return false;
			}
		}
	}
	return true;
}


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
		if (a_event->isEquipping) {
			g_equippedWeaponFormID = form->formID;
			DelayedWeaponTaskDelegate* dlgt = new DelayedWeaponTaskDelegate();
			g_task->AddTask(dlgt);
		} else {
			Visitor visitor;
			VisitPlayerInventoryChanges(&visitor);
		}
		break;
	case RE::FormType::Ammo:
		if (a_event->isEquipping) {
			g_equippedAmmoFormID = form->formID;
			DelayedAmmoTaskDelegate* dlgt = new DelayedAmmoTaskDelegate();
			g_task->AddTask(dlgt);
		}
		break;
	}
	return EventResult::kContinue;
}


bool TESEquipEventHandler::Visitor::Accept(RE::InventoryEntryData* a_entry)
{
	if (a_entry->type && a_entry->type->formID == g_lastEquippedAmmo.GetLoadedFormID() && a_entry->extraList) {
		RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		equipManager->UnEquipItem(player, a_entry->type, a_entry->extraList->front(), a_entry->countDelta, 0, true, false, true, false, 0);
		return false;
	}
	return true;
}


SKSETaskInterface* g_task = 0;
TESEquipEventHandler g_equipEventHandler;
