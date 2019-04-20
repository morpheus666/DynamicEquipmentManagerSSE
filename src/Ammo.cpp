#include "Ammo.h"

#include "Forms.h"  // WeapTypeBoundArrow
#include "PlayerUtil.h"  // PlayerIsBeastRace

#include "SKSE/API.h"
#include "RE/Skyrim.h"


namespace Ammo
{
	Ammo* Ammo::GetSingleton()
	{
		static Ammo singleton;
		return &singleton;
	}


	RE::TESAmmo* Ammo::GetForm()
	{
		return static_cast<RE::TESAmmo*>(ISerializableForm::GetForm());
	}


	void DelayedWeaponTaskDelegate::Run()
	{
		if (g_equippedWeaponFormID != kInvalid) {
			auto weap = RE::TESForm::LookupByID<RE::TESObjectWEAP>(g_equippedWeaponFormID);

			if ((!weap->IsBow() && !weap->IsCrossbow()) || weap->IsBound()) {
				g_equippedWeaponFormID = kInvalid;
				return;
			}

			Visitor visitor;
			VisitPlayerInventoryChanges(&visitor);

			auto ammo = Ammo::GetSingleton()->GetForm();
			if (ammo) {
				auto equipManager = RE::EquipManager::GetSingleton();
				auto player = RE::PlayerCharacter::GetSingleton();
				equipManager->EquipItem(player, ammo, 0, visitor.Count(), 0, true, false, false);

				auto mm = RE::MenuManager::GetSingleton();
				auto uiStrHolder = RE::UIStringHolder::GetSingleton();
				auto invMenu = mm->GetMenu<RE::InventoryMenu>(uiStrHolder->inventoryMenu);
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


	DelayedWeaponTaskDelegate::Visitor::Visitor() :
		_count(0)
	{}


	bool DelayedWeaponTaskDelegate::Visitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		if (a_entry->type->IsAmmo()) {
			if (a_entry->type->formID == Ammo::GetSingleton()->GetFormID()) {
				_count = a_count;
				return false;
			}
		}
		return true;
	}


	SInt32 DelayedWeaponTaskDelegate::Visitor::Count() const
	{
		return _count;
	}


	bool DelayedAmmoTaskDelegate::Visitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		if (a_entry->type->formID == g_equippedAmmoFormID && a_entry->extraList) {
			for (auto& xList : *a_entry->extraList) {
				if (xList->HasType(RE::ExtraDataType::kWorn) || xList->HasType(RE::ExtraDataType::kWornLeft)) {
					auto equipManager = RE::EquipManager::GetSingleton();
					auto player = RE::PlayerCharacter::GetSingleton();
					equipManager->UnEquipItem(player, a_entry->type, xList, a_count, 0, true, false);

					auto mm = RE::MenuManager::GetSingleton();
					auto uiStrHolder = RE::UIStringHolder::GetSingleton();
					auto invMenu = mm->GetMenu<RE::InventoryMenu>(uiStrHolder->inventoryMenu);
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
					Ammo::GetSingleton()->SetForm(g_equippedAmmoFormID);
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


	bool TESEquipEventHandler::Visitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		if (a_entry->type->formID == Ammo::GetSingleton()->GetFormID() && a_entry->extraList) {
			auto equipManager = RE::EquipManager::GetSingleton();
			auto player = RE::PlayerCharacter::GetSingleton();
			auto xList = a_entry->extraList->empty() ? 0 : a_entry->extraList->front();
			equipManager->UnEquipItem(player, a_entry->type, xList, a_count, 0, true, false);
			return false;
		}
		return true;
	}


	TESEquipEventHandler* TESEquipEventHandler::GetSingleton()
	{
		static TESEquipEventHandler singleton;
		return &singleton;
	}


	auto TESEquipEventHandler::ReceiveEvent(RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource)
		-> EventResult
	{
		if (!a_event || !a_event->akSource || !a_event->akSource->IsPlayerRef() || PlayerIsBeastRace()) {
			return EventResult::kContinue;
		}

		auto form = RE::TESForm::LookupByID(a_event->formID);
		if (!form) {
			return EventResult::kContinue;
		}

		auto task = SKSE::GetTaskInterface();
		switch (form->formType) {
		case RE::FormType::Weapon:
			if (a_event->isEquipping) {
				g_equippedWeaponFormID = form->formID;
				task->AddTask(new DelayedWeaponTaskDelegate());
			} else {
				Visitor visitor;
				VisitPlayerInventoryChanges(&visitor);
			}
			break;
		case RE::FormType::Ammo:
			if (a_event->isEquipping) {
				g_equippedAmmoFormID = form->formID;
				task->AddTask(new DelayedAmmoTaskDelegate());
			}
			break;
		}
		return EventResult::kContinue;
	}
}
