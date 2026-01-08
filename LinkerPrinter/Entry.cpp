#include "Entry.hpp"
#include "Hooks.hpp"

#include <SPI.h>
#include "Common/Base.hpp"

#include <spdlog/spdlog.h>

// ! SPI metadata
// ========================================

SPI_PLUGINSIDE_SUPPORT(SDK_TARGET_NAME_W L"LinkerPrinter", L"ME3Tweaks", L"4.1.0", SPI_GAME_SDK_TARGET, SPI_VERSION_ANY);
SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;

namespace LinkerPrinter
{
	// ! Global variables
	// ========================================

	std::unique_ptr<Common::ME3TweaksLogger> FileLogger = nullptr;
	std::map<FString, FString> NodePathToFileNameMap;
	bool CanPrint = true;

	// ! Initialization
	// ========================================

	void InitializeGlobals(::LESDK::Initializer& Init)
	{
		Common::InitializeRequiredGlobals(Init);
	}

	void InitializeHooks(::LESDK::Initializer& Init)
	{
		// Install SetLinker hook
		auto const setLinkerPattern = "8b c9 4d 85 d2 74 39 48 85 d2 74 1c 48 8b c1";
		auto const setLinkerTarget = Init.ResolveTyped<t_SetLinker>(::LESDK::Address::FromPostHook(setLinkerPattern));
		SetLinker_orig = (t_SetLinker*)Init.InstallHook("UObject::SetLinker", setLinkerTarget, SetLinker_hook);

		// Install ProcessEvent hook
		auto const processEventTarget = Init.ResolveTyped<t_UObject_ProcessEvent>(BUILTIN_PROCESSEVENT_PHOOK);
		UObject_ProcessEvent_orig = (t_UObject_ProcessEvent*)Init.InstallHook("UObject::ProcessEvent", processEventTarget, UObject_ProcessEvent_hook);
	}
}

// ! SPI implementation
// ========================================
static constexpr auto loggerName = SDK_TARGET_NAME_A "LinkerPrinter";

SPI_IMPLEMENT_ATTACH
{

	::LESDK::Initializer Init{ InterfacePtr, loggerName };

	::LESDK::InitializeConsole();
	auto outputType = Common::ME3TweaksLogger::LogOutput(
		Common::ME3TweaksLogger::LogOutput::OutputToFile |
		Common::ME3TweaksLogger::LogOutput::OutputToConsole
	);
	try
	{
		::LinkerPrinter::FileLogger = std::make_unique<Common::ME3TweaksLogger>(loggerName, outputType, "LinkerPrinter.log");
	}
	catch (const spdlog::spdlog_ex& ex)
	{
		LEASI_ERROR("Failed to create LinkerPrinter.log: {}", ex.what());
		return false;
	}

	// Initialize globals and hooks
	::LinkerPrinter::InitializeGlobals(Init);
	::LinkerPrinter::InitializeHooks(Init);

	// Print user message
	::LinkerPrinter::FileLogger->info(L"Press CTRL + O to dump all objects that have loaded and their source.");

	return true;
}

SPI_IMPLEMENT_DETACH
{
	LEASI_UNUSED(InterfacePtr);

	// Flush and cleanup logger
	if (::LinkerPrinter::FileLogger)
	{
		::LinkerPrinter::FileLogger->flush();
		::LinkerPrinter::FileLogger.reset();
		spdlog::drop(loggerName);
	}

	::LESDK::TerminateConsole();

	return true;
}
