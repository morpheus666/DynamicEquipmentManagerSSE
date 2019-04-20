#include "Shield.h"

#include "skse64_common/Relocation.h"  // RelocPtr
#include "skse64_common/SafeWrite.h"  // SafeWrite64

#include <type_traits>  // typeid

#include "Animations.h"  // Anim, HashAnimation
#include "PlayerUtil.h"  // PlayerIsBeastRace
#include "Settings.h"  // Settings

#include "RE/Skyrim.h"
#include "REL/Relocation.h"
#include "SKSE/API.h"


namespace Shield
{
	Shield* Shield::GetSingleton()
	{
		static Shield singleton;
		return &singleton;
	}


	RE::TESObjectARMO* Shield::GetForm()
	{
		return static_cast<RE::TESObjectARMO*>(ISerializableForm::GetForm());
	}


	ShieldTaskDelegate::ShieldTaskDelegate(bool a_equip) :
		_equip(a_equip)
	{}


	void ShieldTaskDelegate::Run()
	{
		if (_equip) {
			auto player = RE::PlayerCharacter::GetSingleton();
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
		delete this;
	}


	bool ShieldTaskDelegate::ShieldEquipVisitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		if (a_entry->type->formID == Shield::GetSingleton()->GetFormID()) {
			g_skipAnim = true;
			auto shield = static_cast<RE::TESObjectARMO*>(a_entry->type);
			auto equipManager = RE::EquipManager::GetSingleton();
			auto player = RE::PlayerCharacter::GetSingleton();
			auto xList = (a_entry->extraList && !a_entry->extraList->empty()) ? a_entry->extraList->front() : 0;
			equipManager->EquipItem(player, shield, xList, 1, shield->equipmentType, true, false, false);
			return false;
		}
		return true;
	}


	bool ShieldTaskDelegate::ShieldUnEquipVisitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		using FirstPersonFlag = RE::BGSBipedObjectForm::BipedBodyTemplate::FirstPersonFlag;

		if (a_entry->type->formID == Shield::GetSingleton()->GetFormID()) {
			if (a_entry->extraList) {
				for (auto& xList : *a_entry->extraList) {
					if (xList->HasType(RE::ExtraDataType::kWorn)) {
						auto armor = static_cast<RE::TESObjectARMO*>(a_entry->type);
						if (armor->HasPartOf(FirstPersonFlag::kShield)) {
							auto equipManager = RE::EquipManager::GetSingleton();
							auto player = RE::PlayerCharacter::GetSingleton();
							equipManager->UnEquipItem(player, armor, xList, 1, armor->equipmentType, true, false);
							return false;
						}
					}
				}
			}
		}
		return true;
	}


	void AnimGraphSinkDelegate::Run()
	{
		SinkAnimationGraphEventHandler(BSAnimationGraphEventHandler::GetSingleton());
	}


	void AnimGraphSinkDelegate::Dispose()
	{
		delete this;
	}


	TESEquipEventHandler* TESEquipEventHandler::GetSingleton()
	{
		static TESEquipEventHandler singleton;
		return &singleton;
	}


	auto TESEquipEventHandler::ReceiveEvent(RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource)
		-> EventResult
	{
		using FirstPersonFlag = RE::BGSBipedObjectForm::BipedBodyTemplate::FirstPersonFlag;

		if (!a_event || !a_event->akSource || !a_event->akSource->IsPlayerRef() || PlayerIsBeastRace()) {
			return EventResult::kContinue;
		}

		auto armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(a_event->formID);
		if (!armor) {
			return EventResult::kContinue;
		}

		if (armor->HasPartOf(FirstPersonFlag::kShield)) {
			auto shield = Shield::GetSingleton();
			if (a_event->isEquipping) {
				shield->SetForm(a_event->formID);
			} else {
				auto player = RE::PlayerCharacter::GetSingleton();
				if (player->IsWeaponDrawn()) {
					shield->Clear();
				}
			}
		}

		return EventResult::kContinue;
	}


	BSAnimationGraphEventHandler* BSAnimationGraphEventHandler::GetSingleton()
	{
		static BSAnimationGraphEventHandler singleton;
		return &singleton;
	}


	auto BSAnimationGraphEventHandler::ReceiveEvent(RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource)
		-> EventResult
	{
		if (!a_event || !a_event->akTarget || !a_event->akTarget->IsPlayerRef()) {
			return EventResult::kContinue;
		}

		auto task = SKSE::GetTaskInterface();
		switch (HashAnimation(a_event->animName)) {
		case Anim::kWeaponDraw:
			if (!PlayerIsBeastRace()) {
				task->AddTask(new ShieldTaskDelegate(true));
			}
			break;
		case Anim::kWeaponSheathe:
			if (!PlayerIsBeastRace()) {
				task->AddTask(new ShieldTaskDelegate(false));
			}
			break;
		case Anim::kTailCombatIdle:
			if (!PlayerIsBeastRace()) {
				g_skipAnim = false;
			}
			break;
		case Anim::kGraphDeleting:
			task->AddTask(new AnimGraphSinkDelegate());
			break;
		}

		return EventResult::kContinue;
	}


	class PlayerCharacterEx : public RE::PlayerCharacter
	{
	public:
		using func_t = function_type_t<decltype(&RE::PlayerCharacter::OnItemEquipped)>;
		inline static func_t* func = 0;


		// This hook prevents a double equip anim bug
		void Hook_OnItemEquipped(bool a_playAnim)
		{
			if (g_skipAnim) {
				a_playAnim = false;
			}
			func(this, a_playAnim);
		}


		static void InstallHooks()
		{
			REL::Offset<func_t**> vFunc(RE::Offset::PlayerCharacter::Vtbl + (0xB2 * 0x8));
			func = *vFunc;
			SafeWrite64(vFunc.GetAddress(), GetFnAddr(&Hook_OnItemEquipped));
			_DMESSAGE("[DEBUG] Installed hooks for (%s)", typeid(PlayerCharacterEx).name());
		}
	};


	void InstallHooks()
	{
		PlayerCharacterEx::InstallHooks();
	}
}
