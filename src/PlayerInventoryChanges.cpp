#include "PlayerInventoryChanges.h"

#include "skse64/PluginAPI.h"  // SKSETaskInterface

#include <map>  // map

#include "RE/PlayerCharacter.h"  // PlayerCharacter
#include "RE/InventoryChanges.h"  // InventoryChanges
#include "RE/TESContainer.h"  // TESContainer


class ContainerVisitor
{
public:
	ContainerVisitor(InventoryChangesVisitor* a_visitor, std::map<UInt32, UInt32>* a_changes) :
		_visitor(a_visitor),
		_changes(a_changes)
	{}


	bool Accept(RE::TESContainer::Entry* a_entry)
	{
		if (a_entry->form && a_entry->count > 0) {
			if (_changes->find(a_entry->form->formID) == _changes->end()) {
				RE::InventoryEntryData entry(a_entry->form, a_entry->count);
				return _visitor->Accept(&entry);
			}
		}
		return true;
	}

private:
	InventoryChangesVisitor*	_visitor;
	std::map<UInt32, UInt32>*	_changes;
};


void VisitPlayerInventoryChanges(InventoryChangesVisitor* a_visitor)
{
	RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
	RE::InventoryChanges* changes = player->GetInventoryChanges();
	std::map<UInt32, UInt32> changesMap;
	bool done = false;
	if (changes) {
		for (auto& entry : *changes->entryList) {
			if (entry && entry->type) {
				changesMap.emplace(entry->type->formID, 0);
				if (entry->countDelta > -1) {
					if (!a_visitor->Accept(entry)) {
						done = true;
						break;
					}
				}
			}
		}
	}

	if (done) {
		return;
	}

	RE::TESContainer* container = player->GetContainer();
	if (container) {
		ContainerVisitor visitor(a_visitor, &changesMap);
		container->Visit(visitor);
	}
}


SKSETaskInterface* g_task = 0;
