#include "Hooks.hpp"
#include "Entry.hpp"
#include "Common/Base.hpp"

#include <windows.h>

#include <map>
#include <string>

namespace LinkerPrinter
{
	// ! SetLinker hook
	// ========================================

	t_SetLinker* SetLinker_orig = nullptr;

	void SetLinker_hook(UObject* Context, ULinkerLoad* Linker, int LinkerIndex)
	{
		if (Context->Linker)
		{
			NodePathToFileNameMap.insert_or_assign(Context->GetFullPath(), Context->Linker->Filename);
		}

		// Call original function
		SetLinker_orig(Context, Linker, LinkerIndex);
	}

	// ! UObject::ProcessEvent hook
	// ========================================

	t_UObject_ProcessEvent* UObject_ProcessEvent_orig = nullptr;

	void UObject_ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result)
	{
		// Changed to subclasses of BioHUD to support custom debugger implementation classes of HUD
		if (CanPrint && Function->GetFullName() == L"Function Engine.GameViewportClient.PostRender")
		{
			if ((GetKeyState('O') & 0x8000) && (GetKeyState(VK_CONTROL) & 0x8000))
			{
				PrintLinkers();
				LEASI_TRACE(L"Printed linkers to log");
			}
		}

		// Call original function
		UObject_ProcessEvent_orig(Context, Function, Parms, Result);
	}

	// ! PrintLinkers implementation
	// ========================================

	void PrintLinkers()
	{
		CanPrint = false;

		LEASI_TRACE(L"Dumping {} linkers, this may take a while (game will be unresponsive)...", NodePathToFileNameMap.size());

		// Build reverse lookup map for O(1) performance
		// This prevents O(n^2) complexity when checking if objects still exist
		std::unordered_map<FString, UObject*> ObjectNameToObjectMap;

		for (auto obj : *UObject::GObjObjects)
		{
			if (obj)
			{
				ObjectNameToObjectMap.insert_or_assign(obj->GetFullPath(), obj);
			}
		}

		// Dump all tracked objects
		long numDone = 0;
		for (auto const& [objectPath, filename] : NodePathToFileNameMap)
		{
			// Progress indicator every 1000 objects
			if (numDone % 1000 == 0)
			{
				LEASI_TRACE(L"{} done", numDone);
			}

			// Check if object still exists in memory
			if (ObjectNameToObjectMap.find(objectPath) == ObjectNameToObjectMap.end())
			{
				// Object was garbage collected
				FileLogger->log(L"{} -> (Dumped from memory)", objectPath);
			}
			else
			{
				// Object still exists
				FileLogger->log(L"{} -> {}", objectPath, filename);
			}

			numDone++;
		}

		LEASI_TRACE(L"Printed linker source of {} objects", numDone);
		FileLogger->log(L"------------------------------");
		FileLogger->flush();

		CanPrint = true;
	}
}
