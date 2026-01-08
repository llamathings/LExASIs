#pragma once

#include <LESDK/Headers.hpp>
#include <LESDK/Init.hpp>
#include "Common/ME3TweaksLogger.hpp"

#include <map>
#include <memory>
#include <string>

namespace LinkerPrinter
{
	// ! Global variables
	// ========================================

	extern std::unique_ptr<Common::ME3TweaksLogger> FileLogger;
	extern std::map<FString, FString> NodePathToFileNameMap;
	extern bool CanPrint;

	// ! Initialization functions
	// ========================================

	void InitializeGlobals(::LESDK::Initializer& Init);
	void InitializeHooks(::LESDK::Initializer& Init);
}
