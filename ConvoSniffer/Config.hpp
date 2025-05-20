#pragma once


/// @def    CNVSNF_LOG_MANAGED_FUNCS
/// @brief  Toggles logging of BioConversation calls in ProcessEvent / ProcessInternal / CallFunction hooks.
#ifndef CNVSNF_LOG_MANAGED_FUNCS
    #define CNVSNF_LOG_MANAGED_FUNCS 0
#endif

/// @def    CNVSNF_LOG_NATIVE_FUNCS
/// @brief  Toggles logging of BioConversation calls in native hooks.
#ifndef CNVSNF_LOG_NATIVE_FUNCS
    #define CNVSNF_LOG_NATIVE_FUNCS 0
#endif

/// @def    CNVSNF_PROFILE_CONVO
/// @brief  Toggles debug profiler of the current conversation instance.
#ifndef CNVSNF_PROFILE_CONVO
    #define CNVSNF_PROFILE_CONVO 1
#endif
