#pragma once

#include <LESDK/Headers.hpp>

namespace LinkerPrinter
{
	// ! SetLinker hook
	// ========================================

	using t_SetLinker = void(UObject* Context, ULinkerLoad* Linker, int LinkerIndex);
	extern t_SetLinker* SetLinker_orig;
	void SetLinker_hook(UObject* Context, ULinkerLoad* Linker, int LinkerIndex);

	// ! UObject::ProcessEvent hook
	// ========================================

	using t_UObject_ProcessEvent = void(UObject* Context, UFunction* Function, void* Parms, void* Result);
	extern t_UObject_ProcessEvent* UObject_ProcessEvent_orig;
	void UObject_ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result);

	// ! Helper functions
	// ========================================

	void PrintLinkers();
}
