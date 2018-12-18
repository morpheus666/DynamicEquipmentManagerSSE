#include "PlayerInventoryChanges.h"

#include "skse64/PluginAPI.h"  // SKSETaskInterface

#include "RE/PlayerCharacter.h"  // PlayerCharacter
#include "RE/InventoryChanges.h"  // InventoryChanges


void VisitPlayerInventoryChanges(InventoryChangesVisitor* a_visitor)
{
	RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
	RE::InventoryChanges* changes = player->GetInventoryChanges();
	if (changes) {
		for (auto& entry : *changes->entryList) {
			if (entry) {
				if (!a_visitor->Accept(entry)) {
					break;
				}
			}
		}
	}
}


SKSETaskInterface* g_task = 0;
