#include "skse64/PluginAPI.h"  // PluginHandle, SKSEMessagingInterface, SKSEInterface, PluginInfo, SKSETaskInterface, SKSESerializationInterface
#include "skse64_common/skse_version.h"  // RUNTIME_VERSION

#include <exception> // exception

#include <ShlObj.h>  // CSIDL_MYDOCUMENTS

#include "Ammo.h"  // g_lastEquippedAmmo
#include "Events.h"  // g_equipEventHandler, g_task
#include "Exceptions.h"  // bad_record_info, bad_record_read, bad_ammo_save, bad_ammo_load
#include "json.hpp"  // json
#include "version.h"  // DYNAMICARROWMANAGERSSE_VERSION_VERSTRING

#include "RE/ScriptEventSourceHolder.h"  // ScriptEventSourceHolder


static PluginHandle					g_pluginHandle = kPluginHandle_Invalid;
static SKSEMessagingInterface*		g_messaging = 0;
static SKSESerializationInterface*	g_serialization = 0;


void SaveCallback(SKSESerializationInterface* a_intfc)
{
	using nlohmann::json;

	try {
		json save;
		if (!g_lastEquippedAmmo.Save(save)) {
			throw bad_ammo_save();
		}

		std::string buf = save.dump(4);
#if _DEBUG
		_DMESSAGE("\n[DEBUG] SERIALIZATION SAVE DUMP");
		_DMESSAGE("%s\n", buf.c_str());
#endif
		g_serialization->WriteRecord(g_lastEquippedAmmo.ClassType(), g_lastEquippedAmmo.ClassVersion(), buf.c_str(), buf.length() + 1);
	} catch (std::exception& e) {
		_ERROR("[ERROR] %s", e.what());
	}

	_MESSAGE("[MESSAGE] Finished saving data");
}


void LoadCallback(SKSESerializationInterface* a_intfc)
{
	using nlohmann::json;

	UInt32 type;
	UInt32 version;
	UInt32 length;
	char* buf = 0;

	g_lastEquippedAmmo.Clear();

	try {
		if (!a_intfc->GetNextRecordInfo(&type, &version, &length)) {
			throw bad_record_info();
		}

		buf = new char[length];
		if (!a_intfc->ReadRecordData(buf, length)) {
			throw bad_record_read();
		}

		json load = json::parse(buf);

#if _DEBUG
		_DMESSAGE("\n[DEBUG] SERIALIZATION LOAD DUMP");
		_DMESSAGE("%s\n", load.dump(4).c_str());
#endif

		if (!g_lastEquippedAmmo.Load(load)) {
			throw bad_ammo_load();
		}

	} catch (std::exception& e) {
		_ERROR("[ERROR] %s\n", e.what());
		g_lastEquippedAmmo.Clear();
	}

	delete buf;
	buf = 0;

	_MESSAGE("[MESSAGE] Finished loading data");
}


void MessageHandler(SKSEMessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSEMessagingInterface::kMessage_DataLoaded:
	{
		RE::ScriptEventSourceHolder* sourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		sourceHolder->equipEventSource.AddEventSink(&g_equipEventHandler);
		_MESSAGE("[MESSAGE] Registered equip event handler");
		break;
	}
	}
}


extern "C" {
	bool SKSEPlugin_Query(const SKSEInterface* a_skse, PluginInfo* a_info)
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\DynamicArrowManagerSSE.log");
		gLog.SetPrintLevel(IDebugLog::kLevel_DebugMessage);
		gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

		_MESSAGE("DynamicArrowManagerSSE v%s", DYNAMICARROWMANAGERSSE_VERSION_VERSTRING);

		a_info->infoVersion = PluginInfo::kInfoVersion;
		a_info->name = "DynamicArrowManagerSSE";
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
		_MESSAGE("[MESSAGE] DynamicArrowManagerSSE loaded");

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
			g_serialization->SetUniqueID(g_pluginHandle, 'DNAM');
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
