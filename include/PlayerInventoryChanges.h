#pragma once

#include "skse64/PluginAPI.h"  // SKSETaskInterface

#include "RE/InventoryEntryData.h"  // InventoryEntryData


class InventoryChangesVisitor
{
public:
	virtual bool Accept(RE::InventoryEntryData* a_entry, SInt32 a_count) = 0;
};


void VisitPlayerInventoryChanges(InventoryChangesVisitor* a_visitor);


extern SKSETaskInterface* g_task;
