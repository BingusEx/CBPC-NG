#include "Hooks/Hooks.hpp"

#include "Console/ConsoleManager.hpp"

#include "Hooks/Util/HookUtil.hpp"
#include "CBPC/Actor.hpp"

namespace Hooks {

	/* CBPC Used DetrourXS to hook:
	 * Main::Update,
	 * D3DPresent
	 * BSTaskPool::ProcessEventQueue
	 * These have been converted to call based hooks for improved compatibility.
	 */

	/*
	SE ID: 35565 SE Offset: 0x1e
	AE ID: 36564 AE Offset: 0x3e
	*/
	struct MainUpdate_Begin {

		static int32_t thunk(uint64_t unk) {
			CBP::updateActors();
		
			return func(unk);;
		}

		FUNCTYPE_CALL func;

	};

	/*
		SE ID: 35916 SE Offset: 0x23
		AE ID: 36891 AE Offset: 0x26
	*/

	struct BSTaskPool_ProcessTaskQueue {

		static uint64_t thunk(uint64_t unk) {

			//ActorUpdate();
			return func(unk);
		}

		FUNCTYPE_CALL func;

	};

	struct DXGISwapChain_Present {

		static void thunk(uint32_t unk_01) {

			func(unk_01);

		}

		FUNCTYPE_CALL func;

	};

	struct ConsoleScriptCompiler {

		static void thunk(RE::Script* a_script, RE::ScriptCompiler* a_compiler, RE::COMPILER_NAME a_name, RE::TESObjectREFR* a_targetRef) {

			{
				//logger::info("Entered Console Text: \"{}\"", a_script->text);
				//If true text was a plugin command 
				if (CBP::ConsoleManager::Process(a_script->text)) {
					return;
				}
			}

			func(a_script, a_compiler, a_name, a_targetRef);

		}

		FUNCTYPE_CALL func;
	};


	void Install() {

		auto& SKSETrampoline = SKSE::GetTrampoline();
		SKSETrampoline.create(128);

		stl::write_call<MainUpdate_Begin>(REL::RelocationID(35565,36564, NULL), REL::VariantOffset(0x1e,0x3e, NULL));
		//stl::write_call<DXGISwapChain_Present>(REL::RelocationID(75461, 77246, NULL), REL::VariantOffset(0x9, 0x9, NULL));
		//stl::write_call<BSTaskPool_ProcessTaskQueue>(REL::RelocationID(35916, 36891, NULL), REL::VariantOffset(0x23, 0x26, NULL));
	}




}
