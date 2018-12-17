#pragma once

#include "skse64/gamethreads.h"  // TaskDelegate

#include "Ammo.h"  // Ammo

#include "RE/BSTEvent.h"  // BSTEventSink, EventResult, BSTEventSource
#include "RE/Memory.h"  // TES_HEAP_REDEFINE_NEW
#include "RE/TESEquipEvent.h"  // TESEquipEvent

struct SKSETaskInterface;

namespace RE
{
	class BaseExtraList;
	class InventoryEntryData;
}


enum
{
	kInvalid = 0xFFFFFFFF
};


class InventoryChangesVisitor
{
public:
	virtual bool Accept(RE::InventoryEntryData* a_entry) = 0;
};


void VisitPlayerInventoryChanges(InventoryChangesVisitor* a_visitor);


class DelayedWeaponTaskDelegate : public TaskDelegate
{
public:
	class Visitor : public InventoryChangesVisitor
	{
	public:
		constexpr Visitor() :
			_extraList(0),
			_count(0)
		{}

		virtual bool					Accept(RE::InventoryEntryData* a_entry) override;
		constexpr RE::BaseExtraList*	ExtraList() const { return _extraList; }
		constexpr SInt32				Count() const { return _count; }

	private:
		RE::BaseExtraList*	_extraList;
		SInt32				_count;
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
		virtual bool Accept(RE::InventoryEntryData* a_entry) override;
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
		virtual bool Accept(RE::InventoryEntryData* a_entry) override;
	};


	virtual RE::EventResult ReceiveEvent(RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource) override;
};


static UInt32 g_equippedAmmoFormID = kInvalid;
static UInt32 g_equippedWeaponFormID = kInvalid;

extern SKSETaskInterface* g_task;
extern TESEquipEventHandler g_equipEventHandler;
