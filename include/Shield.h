#pragma once

#include "skse64/gamethreads.h"  // TaskDelegate

#include "ISerializableForm.h"  // ISerializableForm
#include "json.hpp"  // json
#include "PlayerUtil.h"  // InventoryChangesVisitor

#include "RE/BSAnimationGraphEvent.h"  // BSAnimationGraphEvent
#include "RE/BSTEvent.h"  // BSTEventSink, EventResult, BSTEventSource
#include "RE/FormTypes.h"  // TESObjectARMO, EnchantmentItem
#include "RE/Memory.h"  // TES_HEAP_REDEFINE_NEW
#include "RE/TESEquipEvent.h"  // TESEquipEvent


namespace Shield
{
	class Shield : public ISerializableForm
	{
	public:
		Shield();
		virtual ~Shield();

		virtual const char*	ClassName() const override;
		RE::TESObjectARMO*	GetArmorForm();
	};


	class ShieldTaskDelegate : public TaskDelegate
	{
	public:
		class ShieldEquipVisitor : public InventoryChangesVisitor
		{
		public:
			virtual bool Accept(RE::InventoryEntryData* a_entry, SInt32 a_count) override;
		};


		class ShieldUnEquipVisitor : public InventoryChangesVisitor
		{
		public:
			virtual bool Accept(RE::InventoryEntryData* a_entry, SInt32 a_count) override;
		};


		constexpr ShieldTaskDelegate(bool a_equip) :
			_equip(a_equip)
		{}

		virtual void Run() override;
		virtual void Dispose() override;

		TES_HEAP_REDEFINE_NEW();

	private:
		bool _equip;
	};


	class AnimGraphSinkDelegate : public TaskDelegate
	{
	public:
		constexpr AnimGraphSinkDelegate()
		{}

		virtual void Run() override;
		virtual void Dispose() override;

		TES_HEAP_REDEFINE_NEW();
	};


	class TESEquipEventHandler : public RE::BSTEventSink<RE::TESEquipEvent>
	{
	protected:
		TESEquipEventHandler();
		virtual ~TESEquipEventHandler();

	public:
		virtual RE::EventResult ReceiveEvent(RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource) override;

		static TESEquipEventHandler* GetSingleton();
		static void Free();

	protected:
		static TESEquipEventHandler* _singleton;
	};


	class BSAnimationGraphEventHandler : public RE::BSTEventSink<RE::BSAnimationGraphEvent>
	{
	protected:
		BSAnimationGraphEventHandler();
		virtual ~BSAnimationGraphEventHandler();

	public:
		virtual RE::EventResult ReceiveEvent(RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) override;

		static BSAnimationGraphEventHandler* GetSingleton();
		static void Free();

	protected:
		static BSAnimationGraphEventHandler* _singleton;
	};


	void InstallHooks();


	static bool g_skipAnim = false;

	extern Shield g_lastEquippedShield;
}
