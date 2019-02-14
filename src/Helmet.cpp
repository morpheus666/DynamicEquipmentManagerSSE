#include "Helmet.h"

#include "skse64_common/Relocation.h"  // RelocPtr
#include "skse64_common/SafeWrite.h"  // SafeWrite64

#include <type_traits>  // typeid

#include "Animations.h"  // Anim, HashAnimation
#include "Settings.h"  // Settings
#include "version.h"  // MAKE_STR

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


#include "RE/BSTSmartPointer.h"  // TODO
#include "RE/BSAnimationGraphManager.h"  // TODO


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


	void Helmet::Clear()
	{
		ISerializableForm::Clear();
		_enchantment.Clear();
	}


	bool Helmet::Save(json& a_save)
	{
		try {
			json helmetSave;
			if (!ISerializableForm::Save(helmetSave)) {
				return false;
			}

			json enchSave;
			if (!_enchantment.Save(enchSave)) {
				return false;
			}

			a_save = {
				{ ClassName(), helmetSave },
				{ _enchantment.ClassName(), enchSave }
			};
		} catch (std::exception& e) {
			_ERROR("[ERROR] %s\n", e.what());
			return false;
		}

		return true;
	}


	bool Helmet::Load(json& a_load)
	{
		try {
			auto& it = a_load.find(ClassName());
			if (it == a_load.end() || !ISerializableForm::Load(*it)) {
				return false;
			}

			it = a_load.find(_enchantment.ClassName());
			if (it == a_load.end() || !_enchantment.Load(*it)) {
				return false;
			}
		} catch (std::exception& e) {
			_ERROR("[ERROR] %s\n", e.what());
			return false;
		}

		return true;
	}


	void Helmet::SetEnchantmentForm(UInt32 a_formID)
	{
		_enchantment.SetForm(a_formID);
	}


	UInt32 Helmet::GetLoadedEnchantmentFormID()
	{
		return _enchantment.GetLoadedFormID();
	}


	RE::TESObjectARMO* Helmet::GetArmorForm()
	{
		if (GetLoadedFormID() == kInvalid) {
			return 0;
		} else {
			return RE::TESForm::LookupByID<RE::TESObjectARMO>(_loadedFormID);
		}
	}


	RE::EnchantmentItem* Helmet::GetEnchantmentForm()
	{
		return _enchantment.GetEnchantmentForm();
	}


	Helmet::Enchantment::Enchantment() :
		ISerializableForm()
	{}


	Helmet::Enchantment::~Enchantment()
	{}


	const char*	Helmet::Enchantment::ClassName() const
	{
		return MAKE_STR(Enchantment);
	}


	RE::EnchantmentItem* Helmet::Enchantment::GetEnchantmentForm()
	{
		if (GetLoadedFormID() == kInvalid) {
			return 0;
		} else {
			return RE::TESForm::LookupByID<RE::EnchantmentItem>(_loadedFormID);
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


	bool HelmetTaskDelegate::HelmetEquipVisitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		typedef RE::BGSBipedObjectForm::BipedBodyTemplate::FirstPersonFlag FirstPersonFlag;

		if (a_entry->type->formID == g_lastEquippedHelmet.GetLoadedFormID()) {
			RE::TESObjectARMO* armor = static_cast<RE::TESObjectARMO*>(a_entry->type);
			RE::EnchantmentItem* enchantment = g_lastEquippedHelmet.GetEnchantmentForm();
			if (enchantment) {
				if (armor->objectEffect) {
					if (armor->objectEffect->formID != enchantment->formID) {
						return true;
					}
				} else {
					if (!a_entry->extraList) {
						return true;
					}
					bool found = false;
					for (auto& xList : *a_entry->extraList) {
						if (xList->HasType(RE::ExtraDataType::kEnchantment)) {
							RE::ExtraEnchantment* ench = xList->GetByType<RE::ExtraEnchantment>();
							if (ench && ench->objectEffect && ench->objectEffect->formID == enchantment->formID) {
								found = true;
								break;
							}
						}
					}
					if (!found) {
						return true;
					}
				}
			}
			RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
			RE::BaseExtraList* xList = (a_entry->extraList && !a_entry->extraList->empty()) ? a_entry->extraList->front() : 0;
			equipManager->EquipItem(player, armor, xList, 1, armor->equipmentType, true, false, false);
			return false;
		}
		return true;
	}


	bool HelmetTaskDelegate::HelmetUnEquipVisitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		typedef RE::BGSBipedObjectForm::BipedBodyTemplate::FirstPersonFlag FirstPersonFlag;

		if (a_entry->type && a_entry->type->Is(RE::FormType::Armor) && a_entry->extraList) {
			for (auto& xList : *a_entry->extraList) {
				if (xList->HasType(RE::ExtraDataType::kWorn)) {
					RE::TESObjectARMO* armor = static_cast<RE::TESObjectARMO*>(a_entry->type);
					if (armor->HasPartOf(FirstPersonFlag::kHair) && (armor->IsLightArmor() || armor->IsHeavyArmor())) {
						RE::EquipManager* equipManager = RE::EquipManager::GetSingleton();
						RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
						equipManager->UnEquipItem(player, armor, xList, 1, armor->equipmentType, true, false);
						return false;
					}
				}
			}
		}
		return true;
	}


	void UnequipHelmet()
	{}


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


	bool DelayedHelmetLocator::Visitor::Accept(RE::InventoryEntryData* a_entry, SInt32 a_count)
	{
		if (a_entry->type->formID == _formID && a_entry->extraList) {
			for (auto& xList : *a_entry->extraList) {
				if (xList->HasType(RE::ExtraDataType::kWorn)) {
					g_lastEquippedHelmet.Clear();
					g_lastEquippedHelmet.SetForm(_formID);
					RE::TESObjectARMO* armor = static_cast<RE::TESObjectARMO*>(a_entry->type);
					if (armor->objectEffect) {
						g_lastEquippedHelmet.SetEnchantmentForm(armor->objectEffect->formID);
					} else {
						for (auto& xList : *a_entry->extraList) {
							if (xList->HasType(RE::ExtraDataType::kEnchantment)) {
								RE::ExtraEnchantment* ench = xList->GetByType<RE::ExtraEnchantment>();
								if (ench && ench->objectEffect) {
									g_lastEquippedHelmet.SetEnchantmentForm(ench->objectEffect->formID);
								}
							}
						}
					}
					return false;
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


	TESEquipEventHandler::TESEquipEventHandler()
	{}


	TESEquipEventHandler::~TESEquipEventHandler()
	{}


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

		if (armor->HasPartOf(FirstPersonFlag::kHair)) {
			if (armor->IsLightArmor() || armor->IsHeavyArmor()) {
				if (a_event->isEquipping) {
					DelayedHelmetLocator* dlgt = new DelayedHelmetLocator(armor->formID);
					g_task->AddTask(dlgt);
				} else {
					if (player->IsWeaponDrawn()) {
						g_lastEquippedHelmet.Clear();
					}
				}
			} else {
				g_lastEquippedHelmet.Clear();
			}
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


	BSAnimationGraphEventHandler::BSAnimationGraphEventHandler()
	{}


	BSAnimationGraphEventHandler::~BSAnimationGraphEventHandler()
	{}


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
				TaskDelegate* dlgt = new HelmetTaskDelegate(true);
				g_task->AddTask(dlgt);
			}
			break;
		case Anim::kWeaponSheathe:
			{
				TaskDelegate* dlgt = new HelmetTaskDelegate(false);
				g_task->AddTask(dlgt);
			}
			break;
		case Anim::kGraphDeleting:
			{
				TaskDelegate* dlgt = new AnimGraphSinkDelegate();
				g_task->AddTask(dlgt);
			}
			break;
		}

		return EventResult::kContinue;
	}


	BSAnimationGraphEventHandler* BSAnimationGraphEventHandler::GetSingleton()
	{
		if (!_singleton) {
			_singleton = new BSAnimationGraphEventHandler();
		}
		return _singleton;
	}


	void BSAnimationGraphEventHandler::Free()
	{
		delete _singleton;
		_singleton = 0;
	}


	BSAnimationGraphEventHandler* BSAnimationGraphEventHandler::_singleton = 0;


	Helmet g_lastEquippedHelmet;
}
