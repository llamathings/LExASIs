#include "Hooks.hpp"

namespace KismetLogger
{
	// ! Global variables
	// ========================================

	std::unique_ptr<Common::ME3TweaksLogger> FileLogger = nullptr;

	// ! UObject::ProcessEvent hook
	// ========================================

	t_UObject_ProcessEvent* UObject_ProcessEvent_orig = nullptr;
	void UObject_ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result)
	{
		// Check if this is a SequenceOp.Activated() call
		auto functionName = Function->GetFullName();
		if (functionName.Equals(L"Function Engine.SequenceOp.Activated") && FileLogger)
		{
			const auto op = reinterpret_cast<USequenceOp*>(Context);
			auto fullPath = op->GetFullPath();
			auto className = op->Class->Name.ToString();

			// Find which input pin has the impulse
			bool hasInputPin = false;
			FString inputPin;

			for (auto& link : op->InputLinks)
			{
				if (link.bHasImpulse)
				{
					inputPin = link.LinkDesc;
					hasInputPin = true;
					break;
				}
			}

			// Log with or without input pin name
			if (hasInputPin)
			{
				FileLogger->log(L"{} {} {}", className, fullPath, inputPin);
			}
			else
			{
				FileLogger->log(L"{} {}", className, fullPath);
			}

			FileLogger->flush();
		}

		UObject_ProcessEvent_orig(Context, Function, Parms, Result);
	}

	// ! UObject::ProcessInternal hook
	// ========================================

	t_UObject_ProcessInternal* UObject_ProcessInternal_orig = nullptr;
	void UObject_ProcessInternal_hook(UObject* Context, FFrame* Stack, void* Result)
	{
		auto func = Stack->Node;
		auto obj = Stack->Object;

		// Check if this is a native SequenceOp.Activated() call
		auto funcName = func->GetName();
		if (obj->IsA(USequenceOp::StaticClass()) && funcName.Equals(L"Activated") && FileLogger)
		{
			const auto op = reinterpret_cast<USequenceOp*>(Context);
			auto fullPath = op->GetFullPath();
			auto className = op->Class->Name.ToString();

			FileLogger->log(L"{} {}", className, fullPath);
			FileLogger->flush();
		}

		UObject_ProcessInternal_orig(Context, Stack, Result);
	}
}
