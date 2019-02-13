#pragma once

#include "skse64/PluginAPI.h"  // SKSETaskInterface

#include "RE/BSAnimationGraphEvent.h"  // BSAnimationGraphEvent
#include "RE/BSTEvent.h"  // BSTEventSink
#include "RE/InventoryEntryData.h"  // InventoryEntryData
#include "RE/TESObjectLoadedEvent.h"  // TESObjectLoadedEvent


class InventoryChangesVisitor
{
public:
	virtual bool Accept(RE::InventoryEntryData* a_entry, SInt32 a_count) = 0;
};


void VisitPlayerInventoryChanges(InventoryChangesVisitor* a_visitor);
bool SinkAnimationGraphEventHandler(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink);


extern SKSETaskInterface* g_task;
