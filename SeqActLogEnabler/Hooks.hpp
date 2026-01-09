#pragma once

#include <memory>
#include <LESDK/Headers.hpp>
#include <LESDK/Init.hpp>
#include "Common/Base.hpp"
#include "Common/ME3TweaksLogger.hpp"
#include "SeqActLogEnabler/ScreenLogger.hpp"

namespace SeqActLogEnabler
{
    // ! Dedicated file logger
    // ========================================

    extern std::unique_ptr<Common::ME3TweaksLogger> FileLogger;

    // ! Screen logger for on-screen display
    // ========================================

    extern std::unique_ptr<ScreenLogger> OnScreenLogger;

    // ! UObject::ProcessEvent hook for intercepting SeqAct_Log and BioHUD.PostRender
    // ========================================

    using t_UObject_ProcessEvent = void(UObject* Context, UFunction* Function, void* Parms, void* Result);
    extern t_UObject_ProcessEvent* UObject_ProcessEvent_orig;
    void UObject_ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result);
}
