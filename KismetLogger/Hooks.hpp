#pragma once

#include <memory>
#include <LESDK/Headers.hpp>
#include <LESDK/Init.hpp>
#include "Common/Base.hpp"
#include "Common/ME3TweaksLogger.hpp"

namespace KismetLogger
{
    // ! Dedicated file logger
    // ========================================

    extern std::unique_ptr<Common::ME3TweaksLogger> FileLogger;

    // ! UObject::ProcessEvent hook for logging UnrealScript Activated() calls.
    // ========================================

    using t_UObject_ProcessEvent = void(UObject* Context, UFunction* Function, void* Parms, void* Result);
    extern t_UObject_ProcessEvent* UObject_ProcessEvent_orig;
    void UObject_ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result);

    // ! UObject::ProcessInternal hook for logging native Activated() calls.
    // ========================================

    using t_UObject_ProcessInternal = void(UObject* Context, FFrame* Stack, void* Result);
    extern t_UObject_ProcessInternal* UObject_ProcessInternal_orig;
    void UObject_ProcessInternal_hook(UObject* Context, FFrame* Stack, void* Result);
}
