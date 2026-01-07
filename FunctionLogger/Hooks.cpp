#include "Hooks.hpp"

namespace FunctionLogger
{
	// ! Global variables
	// ========================================

	std::unique_ptr<Common::ME3TweaksLogger> FileLogger = nullptr;

	// ! UObject::ProcessEvent hook
	// ========================================

	t_UObject_ProcessEvent* UObject_ProcessEvent_orig = nullptr;
	void UObject_ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result)
	{
		if (FileLogger)
		{
			auto contextPath = Context->GetFullPath();
			auto functionName = Function->GetFullName();

			FileLogger->log(L"[U] {}->{}()", contextPath, functionName);
			FileLogger->flush();
		}

		UObject_ProcessEvent_orig(Context, Function, Parms, Result);
	}

	// ! UObject::ProcessInternal hook
	// ========================================

	t_UObject_ProcessInternal* UObject_ProcessInternal_orig = nullptr;
	void UObject_ProcessInternal_hook(UObject* Context, FFrame* Stack, void* Result)
	{
		if (FileLogger)
		{
			auto contextPath = Context->GetFullPath();
			auto nodeName = Stack->Node->GetFullName();

			FileLogger->log(L"[N] {}->{}()", contextPath, nodeName);
			FileLogger->flush();
		}

		UObject_ProcessInternal_orig(Context, Stack, Result);
	}
}
