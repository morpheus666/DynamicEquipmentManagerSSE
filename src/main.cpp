#include "skse64/PluginAPI.h"  // PluginHandle, SKSEMessagingInterface, SKSEInterface, PluginInfo, SKSETaskInterface, SKSESerializationInterface
#include "skse64_common/skse_version.h"  // RUNTIME_VERSION

#include <exception> // exception

#include <ShlObj.h>  // CSIDL_MYDOCUMENTS

#include "Ammo.h"  // g_lastEquippedAmmo, g_equipEventSink
#include "Exceptions.h"  // bad_record_info, bad_record_version, bad_record_read, bad_ammo_save, bad_helmet_save, bad_shield_save, bad_ammo_load, bad_helmet_load, bad_shield_load
#include "Helmet.h"  // g_lastEquippedHelmet, g_equipEventSink
#include "json.hpp"  // json
#include "PlayerUtil.h"  // g_task, SinkAnimationGraphEventHandler
#include "Settings.h"  // Settings
#include "Shield.h"  // g_lastEquippedShield
#include "version.h"  // DNEM_VERSION_VERSTRING

#include "RE/Skyrim.h"


static PluginHandle					g_pluginHandle = kPluginHandle_Invalid;
static SKSEMessagingInterface*		g_messaging = 0;
static SKSESerializationInterface*	g_serialization = 0;

constexpr UInt32 SERIALIZATION_VERSION = 2;


void SaveCallback(SKSESerializationInterface* a_intfc)
{
	using nlohmann::json;
	using Ammo::g_lastEquippedAmmo;
	using Helmet::g_lastEquippedHelmet;
	using Shield::g_lastEquippedShield;

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

		json shieldSave;
		if (!g_lastEquippedShield.Save(shieldSave)) {
			g_lastEquippedShield.Clear();
			throw bad_shield_save();
		}

		json save = {
			{ g_lastEquippedAmmo.ClassName(), ammoSave },
			{ g_lastEquippedHelmet.ClassName(), helmetSave },
			{ g_lastEquippedShield.ClassName(), shieldSave }
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
	using Shield::g_lastEquippedShield;

	UInt32 type;
	UInt32 version;
	UInt32 length;
	char* buf = 0;

	g_lastEquippedAmmo.Clear();
	g_lastEquippedHelmet.Clear();
	g_lastEquippedShield.Clear();

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

		it = load.find(g_lastEquippedShield.ClassName());
		if (it == load.end() || !g_lastEquippedShield.Load(*it)) {
			g_lastEquippedShield.Clear();
			throw bad_shield_load();
		}
	} catch (std::exception& e) {
		_ERROR("[ERROR] %s\n", e.what());
	}

	delete buf;
	buf = 0;

	_MESSAGE("[MESSAGE] Finished loading data");
}


class TESObjectLoadedEventHandler : public RE::BSTEventSink<RE::TESObjectLoadedEvent>
{
protected:
	TESObjectLoadedEventHandler()
	{}


	virtual ~TESObjectLoadedEventHandler()
	{}

public:
	virtual RE::EventResult ReceiveEvent(RE::TESObjectLoadedEvent* a_event, RE::BSTEventSource<RE::TESObjectLoadedEvent>* a_eventSource) override
	{
		using RE::EventResult;

		if (!a_event) {
			return EventResult::kContinue;
		}

		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		if (a_event->formID == player->formID) {
			if (Settings::manageHelmet) {
				if (SinkAnimationGraphEventHandler(Helmet::BSAnimationGraphEventHandler::GetSingleton())) {
					_MESSAGE("[MESSAGE] Registered helmet player animation event handler");
				}
			}
			if (Settings::manageShield) {
				if (SinkAnimationGraphEventHandler(Shield::BSAnimationGraphEventHandler::GetSingleton())) {
					_MESSAGE("[MESSAGE] Registered shield player animation event handler");
				}
			}
		}

		return EventResult::kContinue;
	}


	static TESObjectLoadedEventHandler* GetSingleton()
	{
		if (!_singleton) {
			_singleton = new TESObjectLoadedEventHandler();
		}
		return _singleton;
	}


	static void Free()
	{
		delete _singleton;
		_singleton = 0;
	}

protected:
	static TESObjectLoadedEventHandler* _singleton;
};


TESObjectLoadedEventHandler* TESObjectLoadedEventHandler::_singleton = 0;


void MessageHandler(SKSEMessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSEMessagingInterface::kMessage_DataLoaded:
		{
			RE::ScriptEventSourceHolder* sourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
			sourceHolder->objectLoadedEventSource.AddEventSink(TESObjectLoadedEventHandler::GetSingleton());
			if (Settings::manageAmmo) {
				sourceHolder->equipEventSource.AddEventSink(Ammo::TESEquipEventHandler::GetSingleton());
				_MESSAGE("[MESSAGE] Registered ammo equip event handler");
			}
			if (Settings::manageHelmet) {
				sourceHolder->equipEventSource.AddEventSink(Helmet::TESEquipEventHandler::GetSingleton());
				_MESSAGE("[MESSAGE] Registered helmet equip event handler");
			}
			if (Settings::manageShield) {
				sourceHolder->equipEventSource.AddEventSink(Shield::TESEquipEventHandler::GetSingleton());
				_MESSAGE("[MESSAGE] Registered shield equip event handler");
			}
		}
		break;
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

		if (a_skse->runtimeVersion != RUNTIME_VERSION_1_5_73) {
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

		if (Settings::manageShield) {
			Shield::InstallHooks();
			_MESSAGE("[MESSAGE] Installed hooks for shield");
		}

		return true;
	}
};
