
#include "Common/Base.hpp"
#include "KismetLogger/Entry.hpp"
#include "KismetLogger/Hooks.hpp"

constexpr auto loggerName = "KismetLogger";

SPI_PLUGINSIDE_SUPPORT(SDK_TARGET_NAME_W L"KismetLogger", L"ME3Tweaks", L"3.1.0", SPI_GAME_SDK_TARGET, SPI_VERSION_ANY);
SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;


SPI_IMPLEMENT_ATTACH
{
	::LESDK::Initializer Init{ InterfacePtr, SDK_TARGET_NAME_A "KismetLogger" };

	// Initialize console and file logger for KismetLog.txt
	::LESDK::InitializeConsole();
	auto outputType = Common::ME3TweaksLogger::LogOutput(
		Common::ME3TweaksLogger::LogOutput::OutputToFile |
		Common::ME3TweaksLogger::LogOutput::OutputToConsole
	);
	try
	{
		::KismetLogger::FileLogger = std::make_unique<Common::ME3TweaksLogger>(loggerName, outputType, "KismetLog.txt");
	}
	catch (const spdlog::spdlog_ex& ex)
	{
		LEASI_ERROR("Failed to create KismetLog.txt: {}", ex.what());
		return false;
	}

	::KismetLogger::InitializeGlobals(Init);
	::KismetLogger::InitializeHooks(Init);

	return true;
}

SPI_IMPLEMENT_DETACH
{
	LEASI_UNUSED(InterfacePtr);

	// Flush and release file logger
	if (::KismetLogger::FileLogger)
	{
		::KismetLogger::FileLogger->flush();
		::KismetLogger::FileLogger.reset();
		spdlog::drop(loggerName);
	}

	::LESDK::TerminateConsole();
	return true;
}


namespace KismetLogger
{
	void InitializeGlobals(::LESDK::Initializer& Init)
	{
		Common::InitializeRequiredGlobals(Init);

		LEASI_INFO("globals initialized");
	}

	void InitializeHooks(::LESDK::Initializer& Init)
	{
		// UObject::ProcessEvent hook for UnrealScript Activated() logging
		// ----------------------------------------
		auto const UObject_ProcessEvent_target = Init.ResolveTyped<t_UObject_ProcessEvent>(BUILTIN_PROCESSEVENT_PHOOK);
		CHECK_RESOLVED(UObject_ProcessEvent_target);
		UObject_ProcessEvent_orig = (t_UObject_ProcessEvent*)Init.InstallHook("UObject::ProcessEvent", UObject_ProcessEvent_target, UObject_ProcessEvent_hook);
		CHECK_RESOLVED(UObject_ProcessEvent_orig);

		// UObject::ProcessInternal hook for native Activated() logging
		// ----------------------------------------
		auto const UObject_ProcessInternal_target = Init.ResolveTyped<t_UObject_ProcessInternal>(BUILTIN_PROCESSINTERNAL_PHOOK);
		CHECK_RESOLVED(UObject_ProcessInternal_target);
		UObject_ProcessInternal_orig = (t_UObject_ProcessInternal*)Init.InstallHook("UObject::ProcessInternal", UObject_ProcessInternal_target, UObject_ProcessInternal_hook);
		CHECK_RESOLVED(UObject_ProcessInternal_orig);

		LEASI_INFO("hooks initialized");
	}
}
