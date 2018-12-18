#pragma once

#include "skse64/PluginAPI.h"  // SKSETaskInterface

#include "RE/InventoryEntryData.h"  // InventoryEntryData


class InventoryChangesVisitor
{
public:
	virtual bool Accept(RE::InventoryEntryData* a_entry) = 0;
};


void VisitPlayerInventoryChanges(InventoryChangesVisitor* a_visitor);


extern SKSETaskInterface* g_task;
