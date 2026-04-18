
/*
 *
void DoHookOLD();
void DoHookNEW();


//console debug
bool Debug_Execute(const ObScriptParam* paramInfo, ScriptData* scriptData, TESObjectREFR* thisObj,
	TESObjectREFR* containingObj, Script* scriptObj, ScriptLocals* locals, double& result,
	uint32_t& opcodeOffsetPtr)
{
	char buffer[MAX_PATH];
	memset(buffer, 0, MAX_PATH);
	char buffer2[MAX_PATH];
	memset(buffer2, 0, MAX_PATH);

	if (!ObScript_ExtractArgs(paramInfo, scriptData, opcodeOffsetPtr, thisObj, containingObj, scriptObj, locals,
		buffer, buffer2))
	{
		return false;
	}

	

	return false;
}

extern "C"
{
	__declspec(dllexport) SKSEPluginVersionData SKSEPlugin_Version =
	{
		SKSEPluginVersionData::kVersion,

		version,
		"CBPC",

		"Shizof",
		"",
		0,
		0,	// not version independent
		{ CURRENT_RELEASE_RUNTIME, 0 },

		0,	// works with any version of the script extender. you probably do not need to put anything here
	};

	void SetupReceptors()
	{
		_MESSAGE("Building Event Sinks...");

		//Retrieve the SKSEActionEvent dispatcher
		EventDispatcherList* edl = GetEventDispatcherList();
		if (edl)
		{
			g_TESEquipEventDispatcher = (EventDispatcher<TESEquipEvent>*)(&(edl->unk4D0));
			g_TESEquipEventDispatcher->AddEventSink(&g_TESEquipEventHandler);
		}
	}

	void OnSKSEMessage(SKSEMessagingInterface::Message * msg)
	{
		switch (msg->type)
		{
			case SKSEMessagingInterface::kMessage_DataLoaded:
			{
				GameLoad();

				LOG_ERR("Loading Configs");
				loadSystemConfig();
				LOG_ERR("Loaded System config");
				loadMasterConfig();
				LOG_ERR("Loaded MasterConfig");
				loadConfig();
				LOG_ERR("Loaded Bounce configs"); 
				loadBounceInterpolationConfig();
				LOG_ERR("Loaded Bounce Interpolation config");
				loadCollisionConfig();
				LOG_ERR("Loaded Collision config");
				loadExtraCollisionConfig();
				LOG_ERR("Loaded Extra Collision configs");
				loadCollisionInterpolationConfig();
				LOG_ERR("Loaded Collision Interpolation config");
				LoadPlayerCollisionEventConfig();
				LOG_ERR("Loaded Player Collision Event config");
			}
			case SKSEMessagingInterface::kMessage_InputLoaded:
			{
				SetupReceptors();
			}
			break;
		}
	}
	
	bool SKSEPlugin_Load(const SKSEInterface* skse)
	{

		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\CBPC-Collision.log");

		gLog.SetPrintLevel(IDebugLog::kLevel_Error);
		gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

		LOG_INFO("CBPC Physics SKSE Plugin");
		LOG_ERR("Query called");

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = skse->GetPluginHandle();

		LOG_ERR("CBPC Physics SKSE Plugin: {}", versionStr);

		g_task = (SKSETaskInterface*)skse->QueryInterface(kInterface_Task);
		
		g_papyrus = (SKSEPapyrusInterface*)skse->QueryInterface(kInterface_Papyrus);

		g_messaging = (SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);
		g_messaging->RegisterListener(g_pluginHandle, "SKSE", OnSKSEMessage);

		DoHookOLD();

		if (!g_task)
		{
			LOG_ERR("Couldn't get Task interface");
			return false;
		}
		ObScriptCommand* hijackedCommand = nullptr;
		for (ObScriptCommand* iter = g_firstConsoleCommand; iter->opcode < kObScript_NumConsoleCommands + kObScript_ConsoleOpBase; ++iter)
		{
			if (!strcmp(iter->longName, "ShowPivot"))
			{
				hijackedCommand = iter;
				break;
			}
		}
		if (hijackedCommand)
		{
			static ObScriptParam params[1];
			params[0].typeID = ObScriptParam::kType_String;
			params[0].typeStr = "String (optional)";
			params[0].isOptional = 1;
			ObScriptCommand cmd = *hijackedCommand;
			cmd.longName = "cbpconfig";
			cmd.shortName = "cbpc";
			cmd.helpText = "cbpc <reload>";
			cmd.needsParent = 0;
			cmd.numParams = 1;
			cmd.params = params;
			cmd.execute = Debug_Execute;
			cmd.flags = 0;
			SafeWriteBuf(reinterpret_cast<uintptr_t>(hijackedCommand), &cmd, sizeof(cmd));
			LOG_ERR("Console interface Loaded");
		}
		else
		{
			LOG_ERR("Couldn't get Console interface");
		}

		g_modEventDispatcher = reinterpret_cast<EventDispatcher<SKSEModCallbackEvent>*>(g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_ModEvent));
		if (g_modEventDispatcher == NULL)
		{
			_MESSAGE("couldn't get mod event dispatcher");
		}
		bool bSuccess = g_papyrus->Register(RegisterFuncs);

		if (bSuccess) {
			LOG_ERR("Register Succeeded");
		}

		
		LOG_ERR("CBPC Load Complete");
		
		return true;
	}
};

*/
