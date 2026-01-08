
#include "Common/Base.hpp"
#include "FunctionLogger/Entry.hpp"
#include "FunctionLogger/Hooks.hpp"

constexpr auto loggerName = "FunctionLogger";

SPI_PLUGINSIDE_SUPPORT(SDK_TARGET_NAME_W L"FunctionLogger", L"Mgamerz", L"4.1.0", SPI_GAME_SDK_TARGET, SPI_VERSION_ANY);
SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;


SPI_IMPLEMENT_ATTACH
{
	::LESDK::Initializer Init{ InterfacePtr, SDK_TARGET_NAME_A "FunctionLogger" };

	// Initialize dedicated file logger for FunctionLog.log
	auto outputType = Common::ME3TweaksLogger::LogOutput::OutputToFile;
	if (nullptr != std::wcsstr(GetCommandLineW(), L" -functionlogtoconsole")) {
		outputType = Common::ME3TweaksLogger::LogOutput(outputType | Common::ME3TweaksLogger::LogOutput::OutputToConsole);
		::LESDK::InitializeConsole();
	}
	try
	{
		::FunctionLogger::FileLogger = std::make_unique<Common::ME3TweaksLogger>(loggerName, outputType, "FunctionLog.log");
	}
	catch (const spdlog::spdlog_ex& ex)
	{
		LEASI_ERROR("Failed to create FunctionLog.log: {}", ex.what());
		return false;
	}

	::FunctionLogger::InitializeGlobals(Init);
	::FunctionLogger::InitializeHooks(Init);

	return true;
}

SPI_IMPLEMENT_DETACH
{
	LEASI_UNUSED(InterfacePtr);

	// Flush and release file logger
	if (::FunctionLogger::FileLogger)
	{
		::FunctionLogger::FileLogger->flush();
		::FunctionLogger::FileLogger.reset();
		spdlog::drop(loggerName);
	}

	::LESDK::TerminateConsole();
	return true;
}


namespace FunctionLogger
{
	void InitializeGlobals(::LESDK::Initializer& Init)
	{
		Common::InitializeRequiredGlobals(Init);

		LEASI_INFO("globals initialized");
	}

	void InitializeHooks(::LESDK::Initializer& Init)
	{
		// UObject::ProcessEvent hook for UnrealScript function logging
		// ----------------------------------------
		auto const UObject_ProcessEvent_target = Init.ResolveTyped<t_UObject_ProcessEvent>(BUILTIN_PROCESSEVENT_PHOOK);
		CHECK_RESOLVED(UObject_ProcessEvent_target);
		UObject_ProcessEvent_orig = (t_UObject_ProcessEvent*)Init.InstallHook("UObject::ProcessEvent", UObject_ProcessEvent_target, UObject_ProcessEvent_hook);
		CHECK_RESOLVED(UObject_ProcessEvent_orig);

		// UObject::ProcessInternal hook for native function logging
		// ----------------------------------------
		auto const UObject_ProcessInternal_target = Init.ResolveTyped<t_UObject_ProcessInternal>(BUILTIN_PROCESSINTERNAL_PHOOK);
		CHECK_RESOLVED(UObject_ProcessInternal_target);
		UObject_ProcessInternal_orig = (t_UObject_ProcessInternal*)Init.InstallHook("UObject::ProcessInternal", UObject_ProcessInternal_target, UObject_ProcessInternal_hook);
		CHECK_RESOLVED(UObject_ProcessInternal_orig);

		LEASI_INFO("hooks initialized");
		LEASI_WARN("Crash game as soon as possible to keep log size down");
	}
}
