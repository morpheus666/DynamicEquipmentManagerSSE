#include "Shield.h"

#include "skse64_common/Relocation.h"  // RelocPtr
#include "skse64_common/SafeWrite.h"  // SafeWrite64

#include <type_traits>  // typeid

#include "Animations.h"  // Anim, HashAnimation
#include "Settings.h"  // Settings
#include "version.h"  // MAKE_STR

#include "RE/ActorProcessManager.h"  // ActorProcessManager
#include "RE/BaseExtraList.h"  // BaseExtraList
#include "RE/BShkbAnimationGraph.h"  // BShkbAnimationGraph
#include "RE/BSTEvent.h"  // EventResult, BSTEventSource
#include "RE/EnchantmentItem.h"  // EnchantmentItem
#include "RE/EquipManager.h"  // EquipManager
#include "RE/ExtraDataTypes.h"  // ExtraDataType
#include "RE/ExtraEnchantment.h"  // ExtraEnchantment
#include "RE/FormTypes.h"  // FormType
#include "RE/InventoryEntryData.h"  // InventoryEntryData
#include "RE/PlayerCharacter.h"  // PlayerCharacter
#include "RE/TESObjectARMO.h"  // TESObjectARMO


namespace Shield
{
	Shield::Shield() :
		ISerializableForm()
	{}


	Shield::~Shield()
	{}


	const char* Shield::ClassName() const
	{
		return MAKE_STR(Shield);
	}


	RE::TESObjectARMO* Shield::GetArmorForm()
	{
		if (GetLoadedFormID() == kInvalid) {
			return 0;
		} else {
			return RE::TESForm::LookupByID<RE::TESObjectARMO>(_loadedFormID);
		}
	}


	void ShieldTaskDelegate::Run()
	{
		if (_equip) {
			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
			if (!player->processManager->GetEquippedLeftHand()) {
				ShieldEquipVisitor visitor;
				VisitPlayerInventoryChanges(&visitor);
			}
		} else {
			ShieldUnEquipVisitor visitor;
			VisitPlayerInventoryChanges(&visitor);
		}
	}


	void ShieldTaskDelegate::Dispose()
	{
		if (this) {
			delete this;
		}
	}


	bool ShieldTaskDelegate::ShieldEquipVisitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		typedef RE::BGSBipedObjectForm::BipedBodyTemplate::FirstPersonFlag FirstPersonFlag;

		if (a_entry->type->formID == g_lastEquippedShield.GetLoadedFormID()) {
			g_skipAnim = true;
			RE::TESObjectARMO* shield = static_cast<RE::TESObjectARMO*>(a_entry->type);
			RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
			RE::BaseExtraList* xList = (a_entry->extraList && !a_entry->extraList->empty()) ? a_entry->extraList->front() : 0;
			equipManager->EquipItem(player, shield, xList, 1, shield->equipmentType, true, false, false, 0);
			return false;
		}
		return true;
	}


	bool ShieldTaskDelegate::ShieldUnEquipVisitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		typedef RE::BGSBipedObjectForm::BipedBodyTemplate::FirstPersonFlag FirstPersonFlag;

		if (a_entry->type->formID == g_lastEquippedShield.GetLoadedFormID()) {
			if (a_entry->extraList) {
				for (auto& xList : *a_entry->extraList) {
					if (xList->HasType(RE::ExtraDataType::kWorn)) {
						RE::TESObjectARMO* armor = static_cast<RE::TESObjectARMO*>(a_entry->type);
						if (armor->HasPartOf(FirstPersonFlag::kShield)) {
							RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
							RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
							equipManager->UnEquipItem(player, armor, xList, 1, armor->equipmentType, true, false, true, false, 0);
							return false;
						}
					}
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

		RE::TESObjectARMO* armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(a_event->formID);
		if (!armor) {
			return EventResult::kContinue;
		}

		if (armor->HasPartOf(FirstPersonFlag::kShield)) {
			if (a_event->isEquipping) {
				g_lastEquippedShield.SetForm(a_event->formID);
			} else {
				if (player->IsWeaponDrawn()) {
					g_lastEquippedShield.Clear();
				}
			}
		}

		return EventResult::kContinue;
	}


	RE::EventResult BSAnimationGraphEventHandler::ReceiveEvent(RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource)
	{
		using RE::EventResult;

		if (!a_event || !a_event->akTarget) {
			return EventResult::kContinue;
		}

		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		if (a_event->akTarget->formID != player->formID) {
			return EventResult::kContinue;
		}

		switch (HashAnimation(a_event->animName)) {
		case Anim::kWeaponDraw:
			{
				TaskDelegate* dlgt = new ShieldTaskDelegate(true);
				g_task->AddTask(dlgt);
			}
			break;
		case Anim::kWeaponSheathe:
			{
				TaskDelegate* dlgt = new ShieldTaskDelegate(false);
				g_task->AddTask(dlgt);
			}
			break;
		case Anim::kTailCombatIdle:
			g_skipAnim = false;
			break;
		}

		return EventResult::kContinue;
	}


	class PlayerCharacterEx : public RE::PlayerCharacter
	{
	public:
		typedef void _OnItemEquipped_t(RE::PlayerCharacter* a_this, bool a_playAnim);
		static _OnItemEquipped_t* orig_OnItemEquipped;


		// This hook prevents a double equip anim bug
		void Hook_OnItemEquipped(bool a_playAnim)
		{
			if (g_skipAnim) {
				a_playAnim = false;
			}
			orig_OnItemEquipped(this, a_playAnim);
		}


		static void InstallHooks()
		{
			constexpr uintptr_t PLAYER_CHARACTER_VTBL = 0x0167D640;
			RelocPtr<_OnItemEquipped_t*> vtbl_OnItemEquipped(PLAYER_CHARACTER_VTBL + (0xB2 * 0x8));
			orig_OnItemEquipped = *vtbl_OnItemEquipped;
			SafeWrite64(vtbl_OnItemEquipped.GetUIntPtr(), GetFnAddr(&Hook_OnItemEquipped));
			_DMESSAGE("[DEBUG] Installed hooks for (%s)", typeid(PlayerCharacterEx).name());
		}
	};


	PlayerCharacterEx::_OnItemEquipped_t* PlayerCharacterEx::orig_OnItemEquipped;


	void InstallHooks()
	{
		PlayerCharacterEx::InstallHooks();
	}


	Shield g_lastEquippedShield;
	TESEquipEventHandler g_equipEventSink;
	BSAnimationGraphEventHandler g_animationGraphEventSink;
}
