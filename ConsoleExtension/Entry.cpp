#include <spdlog/details/windows_include.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "Common/Base.hpp"
#include "ConsoleExtension/Entry.hpp"
#include "ConsoleExtension/Hooks.hpp"

SPI_PLUGINSIDE_SUPPORT(SDK_TARGET_NAME_W L"ConsoleExtension", L"SirCxyrtyx", L"2.1.0", SPI_GAME_SDK_TARGET, SPI_VERSION_ANY);
SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;


#ifdef SDK_TARGET_LE3
	// Pattern for StaticConstructObject in LE3
	#define PATTERN_STATICCONSTRUCTOBJECT_LE3 ::LESDK::Address::FromPostHook(/* 48 8b c4 48 89 */ "50 10 56 57 41 55 41 56 41 57 48 83 ec 70 48 c7 40 b8 fe ff ff ff")
#endif


SPI_IMPLEMENT_ATTACH
{
	::LESDK::Initializer Init{ InterfacePtr, SDK_TARGET_NAME_A "ConsoleExtension" };

	::ConsoleExtension::InitializeGlobals(Init);
	::ConsoleExtension::InitializeHooks(Init);

	return true;
}

SPI_IMPLEMENT_DETACH
{
	LEASI_UNUSED(InterfacePtr);
#ifdef _DEBUG
	::LESDK::TerminateConsole();
#endif
	return true;
}


namespace ConsoleExtension
{
	void InitializeGlobals(::LESDK::Initializer& Init)
	{
		Common::InitializeRequiredGlobals(Init);

		LEASI_INFO("globals initialized");
	}

	void InitializeHooks(::LESDK::Initializer& Init)
	{
		// UEngine::Exec hook for console commands
		// ----------------------------------------
		auto const UEngine_Exec_target = Init.ResolveTyped<t_UEngine_Exec>(BUILTIN_EXEC_PHOOK);
		CHECK_RESOLVED(UEngine_Exec_target);
		UEngine_Exec_orig = (t_UEngine_Exec*)Init.InstallHook("UEngine::Exec", UEngine_Exec_target, UEngine_Exec_hook);
		CHECK_RESOLVED(UEngine_Exec_orig);

		// UObject::ProcessEvent hook for camera tracking
		// ----------------------------------------
		auto const UObject_ProcessEvent_target = Init.ResolveTyped<t_UObject_ProcessEvent>(BUILTIN_PROCESSEVENT_PHOOK);
		CHECK_RESOLVED(UObject_ProcessEvent_target);
		UObject_ProcessEvent_orig = (t_UObject_ProcessEvent*)Init.InstallHook("UObject::ProcessEvent", UObject_ProcessEvent_target, UObject_ProcessEvent_hook);
		CHECK_RESOLVED(UObject_ProcessEvent_orig);

#ifdef SDK_TARGET_LE3
		// StaticConstructObject for LE3
		// ----------------------------------------
		StaticConstructObject = Init.ResolveTyped<t_StaticConstructObject>(PATTERN_STATICCONSTRUCTOBJECT_LE3);
		CHECK_RESOLVED(StaticConstructObject);
#endif

		LEASI_INFO("hooks initialized");
	}
}
