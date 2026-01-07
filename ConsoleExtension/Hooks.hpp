#pragma once

#include <LESDK/Headers.hpp>
#include <LESDK/Init.hpp>
#include "Common/Base.hpp"

namespace ConsoleExtension
{
    // ! UEngine::Exec hook for console command handling.
    // ========================================

    using t_UEngine_Exec = unsigned(UEngine* Context, wchar_t* cmd, void* unk);
    extern t_UEngine_Exec* UEngine_Exec_orig;
    unsigned UEngine_Exec_hook(UEngine* Context, wchar_t* cmd, void* unk);

    // ! UObject::ProcessEvent hook for event handling.
    // ========================================

    using t_UObject_ProcessEvent = void(UObject* Context, UFunction* Function, void* Parms, void* Result);
    extern t_UObject_ProcessEvent* UObject_ProcessEvent_orig;
    void UObject_ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result);

#ifdef SDK_TARGET_LE3
    // ! StaticConstructObject for LE3 only.
    // ========================================

    using t_StaticConstructObject = void*(UClass* Class, UObject* InOuter, SFXName Name, long long SetFlags, UObject* Template, void* outputDevice, UObject* SubobjectRoot, struct FObjectInstancingGraph* InstanceGraph);
    extern t_StaticConstructObject* StaticConstructObject;
#endif
}
