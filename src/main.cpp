#include "skse64/PluginAPI.h"  // PluginHandle, SKSEMessagingInterface, SKSEInterface, PluginInfo, SKSETaskInterface, SKSESerializationInterface
#include "skse64_common/skse_version.h"  // RUNTIME_VERSION

#include <exception> // exception

#include <ShlObj.h>  // CSIDL_MYDOCUMENTS

#include "Ammo.h"  // g_lastEquippedAmmo, g_equipEventSink
#include "Exceptions.h"  // bad_record_info, bad_record_version, bad_record_read, bad_ammo_save, bad_helmet_save, bad_ammo_load, bad_helmet_load
#include "Helmet.h"  // g_lastEquippedHelmet, g_equipEventSink
#include "HookShare.h"  // HookShare
#include "json.hpp"  // json
#include "PlayerInventoryChanges.h"  // g_task
#include "Settings.h"  // Settings
#include "version.h"  // DNEM_VERSION_VERSTRING

#include "RE/BShkbAnimationGraph.h"  // BShkbAnimationGraph
#include "RE/PlayerCharacter.h"  // PlayerCharacter
#include "RE/ScriptEventSourceHolder.h"  // ScriptEventSourceHolder


static PluginHandle					g_pluginHandle = kPluginHandle_Invalid;
static SKSEMessagingInterface*		g_messaging = 0;
static SKSESerializationInterface*	g_serialization = 0;


constexpr UInt32 SERIALIZATION_VERSION = 2;


void SaveCallback(SKSESerializationInterface* a_intfc)
{
	using nlohmann::json;
	using Ammo::g_lastEquippedAmmo;
	using Helmet::g_lastEquippedHelmet;

	try {
		json ammoSave;
		if (!g_lastEquippedAmmo.Save(ammoSave)) {
			g_lastEquippedAmmo.Clear();
			throw bad_ammo_save();
		}

		json helmetSave;
		if (!g_lastEquippedHelmet.Save(helmetSave)) {
			g_lastEquippedHelmet.Clear();
			throw bad_helmet_save();
		}

		json save = {
			{ g_lastEquippedAmmo.ClassName(), ammoSave },
			{ g_lastEquippedHelmet.ClassName(), helmetSave }
		};

		std::string buf = save.dump(4);
#if _DEBUG
		_DMESSAGE("\nSERIALIZATION SAVE DUMP\n%s\n", buf.c_str());
#endif
		g_serialization->WriteRecord('DNAM', SERIALIZATION_VERSION, buf.c_str(), buf.length() + 1);
	} catch (std::exception& e) {
		_ERROR("[ERROR] %s", e.what());
	}

	_MESSAGE("[MESSAGE] Finished saving data");
}


void LoadCallback(SKSESerializationInterface* a_intfc)
{
	using nlohmann::json;
	using Ammo::g_lastEquippedAmmo;
	using Helmet::g_lastEquippedHelmet;

	UInt32 type;
	UInt32 version;
	UInt32 length;
	char* buf = 0;

	g_lastEquippedAmmo.Clear();
	g_lastEquippedHelmet.Clear();

	try {
		if (!a_intfc->GetNextRecordInfo(&type, &version, &length)) {
			throw bad_record_info();
		}

		if (version != SERIALIZATION_VERSION) {
			throw bad_record_version(SERIALIZATION_VERSION, version);
		}

		buf = new char[length];
		if (!a_intfc->ReadRecordData(buf, length)) {
			throw bad_record_read();
		}

		json load = json::parse(buf);

#if _DEBUG
		_DMESSAGE("\nSERIALIZATION LOAD DUMP\n%s\n", load.dump(4).c_str());
#endif

		auto& it = load.find(g_lastEquippedAmmo.ClassName());
		if (it == load.end() || !g_lastEquippedAmmo.Load(*it)) {
			g_lastEquippedAmmo.Clear();
			throw bad_ammo_load();
		}

		it = load.find(g_lastEquippedHelmet.ClassName());
		if (it == load.end() || !g_lastEquippedHelmet.Load(*it)) {
			g_lastEquippedHelmet.Clear();
			throw bad_helmet_load();
		}
	} catch (std::exception& e) {
		_ERROR("[ERROR] %s\n", e.what());
	}

	delete buf;
	buf = 0;

	_MESSAGE("[MESSAGE] Finished loading data");
}


void RegisterForPlayerAnimationEvent(RE::TESObjectREFR* a_refr, RE::BShkbAnimationGraphPtr& a_animGraph)
{
	static bool sinked = false;
	RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
	if (!sinked && a_refr->formID == player->formID) {
		a_animGraph->GetBSAnimationGraphEventSource()->AddEventSink(&Helmet::g_animationGraphEventSink);
		sinked = true;
	}
	_MESSAGE("[MESSAGE] Registered player animation event handler");
}


void HooksReady(SKSEMessagingInterface::Message* a_msg)
{
	using HookShare::_RegisterForAnimationGraphEvent_t;
	using HookShare::Hook;

	switch (a_msg->type) {
	case HookShare::kType_AnimationGraphEvent:
		if (a_msg->dataLen == HOOK_SHARE_API_VERSION_MAJOR) {
			_RegisterForAnimationGraphEvent_t* _RegisterForAnimationGraphEvent = static_cast<_RegisterForAnimationGraphEvent_t*>(a_msg->data);
			_RegisterForAnimationGraphEvent(RegisterForPlayerAnimationEvent, Hook::kPlayerAnimationGraphEvent);
			_MESSAGE("[MESSAGE] Hooks registered");
		} else {
			_FATALERROR("[FATAL ERROR] An incompatible version of Hook Share SSE was loaded! Expected (%i), found (%i)!\n", HOOK_SHARE_API_VERSION_MAJOR, a_msg->type);
		}
		break;
	}
}


void MessageHandler(SKSEMessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSEMessagingInterface::kMessage_PostPostLoad:
		if (g_messaging->RegisterListener(g_pluginHandle, "HookShareSSE", HooksReady)) {
			_MESSAGE("[MESSAGE] Registered HookShareSSE listener");
		} else {
			_FATALERROR("[FATAL ERROR] HookShareSSE not loaded!\n");
		}
		break;
	case SKSEMessagingInterface::kMessage_DataLoaded:
	{
		RE::ScriptEventSourceHolder* sourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		if (Settings::manageAmmo) {
			sourceHolder->equipEventSource.AddEventSink(&Ammo::g_equipEventSink);
			_MESSAGE("[MESSAGE] Registered ammo equip event handler");
		}
		if (Settings::manageHelmet) {
			sourceHolder->equipEventSource.AddEventSink(&Helmet::g_equipEventSink);
			_MESSAGE("[MESSAGE] Registered helmet equip event handler");
		}
		break;
	}
	}
}


extern "C" {
	bool SKSEPlugin_Query(const SKSEInterface* a_skse, PluginInfo* a_info)
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\DynamicEquipmentManagerSSE.log");
		gLog.SetPrintLevel(IDebugLog::kLevel_DebugMessage);
		gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

		_MESSAGE("DynamicEquipmentManagerSSE v%s", DNEM_VERSION_VERSTRING);

		a_info->infoVersion = PluginInfo::kInfoVersion;
		a_info->name = "DynamicEquipmentManagerSSE";
		a_info->version = 1;

		g_pluginHandle = a_skse->GetPluginHandle();

		if (a_skse->isEditor) {
			_FATALERROR("[FATAL ERROR] Loaded in editor, marking as incompatible!\n");
			return false;
		}

		if (a_skse->runtimeVersion != RUNTIME_VERSION_1_5_62) {
			_FATALERROR("[FATAL ERROR] Unsupported runtime version %08X!\n", a_skse->runtimeVersion);
			return false;
		}

		return true;
	}


	bool SKSEPlugin_Load(const SKSEInterface* a_skse)
	{
		_MESSAGE("[MESSAGE] DynamicEquipmentManagerSSE loaded");

		if (Settings::loadSettings()) {
			_MESSAGE("[MESSAGE] Settings loaded successfully");
#if _DEBUG
			Settings::dump();
#endif
		} else {
			_FATALERROR("[FATAL ERROR] Failed to load settings!\n");
			return false;
		}

		g_task = (SKSETaskInterface*)a_skse->QueryInterface(kInterface_Task);
		if (g_task) {
			_MESSAGE("[MESSAGE] Task interface query successful");
		} else {
			_FATALERROR("[FATAL ERROR] Task interface query failed!\n");
			return false;
		}

		g_messaging = (SKSEMessagingInterface*)a_skse->QueryInterface(kInterface_Messaging);
		if (g_messaging->RegisterListener(g_pluginHandle, "SKSE", MessageHandler)) {
			_MESSAGE("[MESSAGE] Messaging interface registration successful");
		} else {
			_FATALERROR("[FATAL ERROR] Messaging interface registration failed!\n");
			return false;
		}

		g_serialization = (SKSESerializationInterface*)a_skse->QueryInterface(kInterface_Serialization);
		if (g_serialization) {
			g_serialization->SetUniqueID(g_pluginHandle, 'DNEM');
			g_serialization->SetSaveCallback(g_pluginHandle, SaveCallback);
			g_serialization->SetLoadCallback(g_pluginHandle, LoadCallback);
			_MESSAGE("[MESSAGE] Serialization interface query successful");
		} else {
			_FATALERROR("[FATAL ERROR] Serialization interface query failed!\n");
			return false;
		}

		return true;
	}
};
