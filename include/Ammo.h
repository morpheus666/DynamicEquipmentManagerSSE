#pragma once

#include "skse64/gamethreads.h"  // TaskDelegate

#include <string>  // string

#include "ISerializableForm.h"  // ISerializableForm, kInvalid
#include "json.hpp"  // json
#include "PlayerInventoryChanges.h"  // InventoryChangesVisitor

#include "RE/BSTEvent.h"  // BSTEventSink, EventResult, BSTEventSource
#include "RE/FormTypes.h"  // TESAmmo
#include "RE/Memory.h"  // TES_HEAP_REDEFINE_NEW
#include "RE/TESEquipEvent.h"  // TESEquipEvent

namespace RE
{
	class BaseExtraList;
	class InventoryEntryData;
}


namespace Ammo
{
	class Ammo : public ISerializableForm
	{
	public:
		Ammo();
		virtual ~Ammo();

		virtual const char*	ClassName() const override;
		RE::TESAmmo*		GetAmmoForm();
	};


	class DelayedWeaponTaskDelegate : public TaskDelegate
	{
	public:
		class Visitor : public InventoryChangesVisitor
		{
		public:
			constexpr Visitor() :
				_count(0)
			{}

			virtual bool		Accept(RE::InventoryEntryData* a_entry, SInt32 a_count) override;
			constexpr SInt32	Count() const { return _count; }

		private:
			SInt32 _count;
		};

		virtual void Run() override;
		virtual void Dispose() override;

		TES_HEAP_REDEFINE_NEW();
	};


	class DelayedAmmoTaskDelegate : public TaskDelegate
	{
	public:
		class Visitor : public InventoryChangesVisitor
		{
		public:
			virtual bool Accept(RE::InventoryEntryData* a_entry, SInt32 a_count) override;
		};

		virtual void Run() override;
		virtual void Dispose() override;

		TES_HEAP_REDEFINE_NEW();
	};


	class TESEquipEventHandler : public RE::BSTEventSink<RE::TESEquipEvent>
	{
	public:
		class Visitor : public InventoryChangesVisitor
		{
		public:
			virtual bool Accept(RE::InventoryEntryData* a_entry, SInt32 a_count) override;
		};


		virtual RE::EventResult ReceiveEvent(RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource) override;
	};


	static UInt32 g_equippedAmmoFormID = kInvalid;
	static UInt32 g_equippedWeaponFormID = kInvalid;

	extern Ammo g_lastEquippedAmmo;
	extern TESEquipEventHandler g_equipEventSink;
}
