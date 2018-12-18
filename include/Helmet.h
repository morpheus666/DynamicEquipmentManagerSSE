#pragma once

#include "skse64/gamethreads.h"  // TaskDelegate

#include "ISerializableForm.h"  // ISerializableForm
#include "json.hpp"  // json
#include "PlayerInventoryChanges.h"  // InventoryChangesVisitor

#include "RE/BSTEvent.h"  // BSTEventSink, EventResult, BSTEventSource
#include "RE/Memory.h"  // TES_HEAP_REDEFINE_NEW
#include "RE/TESEquipEvent.h"  // TESEquipEvent

namespace RE
{
	class TESObjectARMO;
}


namespace Helmet
{
	class Helmet : public ISerializableForm
	{
	private:
		using json = nlohmann::json;


		enum
		{
			kVersion = 1
		};

	public:
		Helmet();
		virtual ~Helmet();

		virtual const char*	ClassName() const;
		virtual UInt32		ClassVersion() const;
		virtual UInt32		ClassType() const;
		bool				Save(json& a_save);
		bool				Load(json& a_load);
		RE::TESObjectARMO*	GetArmorForm();
	};


	class HelmetTaskDelegate : public TaskDelegate
	{
	public:
		class HelmetEquipVisitor : public InventoryChangesVisitor
		{
		public:
			virtual bool Accept(RE::InventoryEntryData* a_entry) override;
		};


		class HelmetUnEquipVisitor : public InventoryChangesVisitor
		{
		public:
			virtual bool Accept(RE::InventoryEntryData* a_entry) override;
		};


		constexpr HelmetTaskDelegate(bool a_equip) :
			_equip(a_equip)
		{}

		virtual void Run() override;
		virtual void Dispose() override;

		TES_HEAP_REDEFINE_NEW();

	private:
		bool _equip;
	};


	class DelayedHelmetLocator : public TaskDelegate
	{
	public:
		class Visitor : public InventoryChangesVisitor
		{
		public:
			constexpr explicit Visitor(UInt32 a_formID) :
				_formID(a_formID)
			{}

			virtual bool Accept(RE::InventoryEntryData* a_entry) override;

		private:
			UInt32 _formID;
		};


		constexpr explicit DelayedHelmetLocator(UInt32 a_formID) :
			_formID(a_formID)
		{}

		virtual void Run() override;
		virtual void Dispose() override;

		TES_HEAP_REDEFINE_NEW();

	private:
		UInt32 _formID;
	};


	class TESEquipEventHandler : public RE::BSTEventSink<RE::TESEquipEvent>
	{
	public:
		virtual RE::EventResult ReceiveEvent(RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource) override;
	};


	void InstallHooks();


	extern Helmet g_lastEquippedHelmet;
	extern TESEquipEventHandler g_equipEventSink;
}
