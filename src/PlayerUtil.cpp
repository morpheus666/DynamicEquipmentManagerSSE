#include "PlayerUtil.h"

#include "skse64/PluginAPI.h"  // SKSETaskInterface

#include <utility>  // pair, make_pair
#include <map>  // map
#include <vector>  // vector

#include "RE/BSAnimationGraphManager.h"  // BSAnimationGraphManagerPtr
#include "RE/BShkbAnimationGraph.h"  // BShkbAnimationGraph
#include "RE/PlayerCharacter.h"  // PlayerCharacter
#include "RE/InventoryChanges.h"  // InventoryChanges
#include "RE/TESContainer.h"  // TESContainer


typedef UInt32 FormID;
typedef SInt32 Count;


class ContainerVisitor
{
public:
	ContainerVisitor(std::map<FormID, std::pair<RE::InventoryEntryData*, Count>>* a_invMap) :
		_invMap(a_invMap)
	{}


	bool Accept(RE::TESContainer::Entry* a_entry)
	{
		if (a_entry->form) {
			auto& it = _invMap->find(a_entry->form->formID);
			if (it != _invMap->end()) {
				if (!a_entry->form->IsGold()) {
					it->second.second += a_entry->count;
				}
			} else {
				RE::InventoryEntryData* entryData = new RE::InventoryEntryData(a_entry->form, a_entry->count);
				_heapList.push_back(entryData);
				_invMap->emplace(a_entry->form->formID, std::make_pair(entryData, entryData->countDelta));
			}
		}
		return true;
	}


	void Free()
	{
		for (auto& entry : _heapList) {
			delete entry;
			entry = 0;
		}
	}

private:
	std::map<FormID, std::pair<RE::InventoryEntryData*, Count>>*	_invMap;
	std::vector<RE::InventoryEntryData*>							_heapList;
};


void VisitPlayerInventoryChanges(InventoryChangesVisitor* a_visitor)
{
	RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
	RE::InventoryChanges* changes = player->GetInventoryChanges();
	std::map<FormID, std::pair<RE::InventoryEntryData*, Count>> invMap;
	if (changes) {
		for (auto& entry : *changes->entryList) {
			if (entry && entry->type) {
				invMap.emplace(entry->type->formID, std::make_pair(entry, entry->countDelta));
			}
		}
	}

	RE::TESContainer* container = player->GetContainer();
	ContainerVisitor visitor(&invMap);
	if (container) {
		container->Visit(visitor);
	}

	for (auto& item : invMap) {
		if (item.second.second > 0) {
			if (!a_visitor->Accept(item.second.first, item.second.second)) {
				break;
			}
		}
	}

	visitor.Free();
}


bool SinkAnimationGraphEventHandler(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink)
{
	RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
	RE::BSAnimationGraphManagerPtr graphManager;
	player->GetAnimationGraphManager(graphManager);
	if (graphManager) {
		bool sinked = false;
		for (auto& animationGraph : graphManager->animationGraphs) {
			if (sinked) {
				break;
			}
			RE::BSTEventSource<RE::BSAnimationGraphEvent>* eventSource = animationGraph->GetBSAnimationGraphEventSource();
			for (auto& sink : eventSource->eventSinks) {
				if (sink == a_sink) {
					sinked = true;
					break;
				}
			}
		}
		if (!sinked) {
			graphManager->animationGraphs.front()->GetBSAnimationGraphEventSource()->AddEventSink(a_sink);
			return true;
		}
	}
	return false;
}


SKSETaskInterface* g_task = 0;
