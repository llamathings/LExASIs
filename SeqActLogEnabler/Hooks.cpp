#include "Hooks.hpp"
#include <sstream>

namespace SeqActLogEnabler
{
    // ! Global variables
    // ========================================

    std::unique_ptr<Common::ME3TweaksLogger> FileLogger = nullptr;
    std::unique_ptr<ScreenLogger> OnScreenLogger = nullptr;

    // If we should draw the log
	bool DrawLog = true;

	// If we can draw the log right now (to prevent multiple toggles on key hold)
	bool CanToggleDrawLog = false;

    // ! Helper function to resolve SeqVar_External variables
    // ========================================

    static USequenceVariable* ResolveSeqVar_External(USeqVar_External* seqExtObj)
    {
        USequence* parentSeq = seqExtObj->ParentSequence;
        if (parentSeq == nullptr) return nullptr;

        // Find the matching variable link in the parent sequence
        for (auto& varLink : parentSeq->VariableLinks)
        {
            auto linkVarName = varLink.LinkVar.ToString();
            auto extVarName = seqExtObj->VarName.ToString();

            if (linkVarName.Equals(extVarName))
            {
                // Find the linked variable
                for (auto* variable : varLink.LinkedVariables)
                {
                    if (variable != nullptr)
                    {
                        // Recursively resolve if this is also an external variable
                        if (variable->IsA(USeqVar_External::StaticClass()))
                        {
                            return ResolveSeqVar_External(static_cast<USeqVar_External*>(variable));
                        }
                        return variable;
                    }
                }
            }
        }

        return nullptr;
    }

    // ! Helper function to print a sequence variable's value
    // ========================================

    static void PrintSequenceVariable(USequenceVariable* seqVar, std::wstringstream& ss)
    {
        if (seqVar->IsA(USeqVar_External::StaticClass()))
        {
            auto* seqExtObj = static_cast<USeqVar_External*>(seqVar);
            auto* referencedObj = ResolveSeqVar_External(seqExtObj);
            if (referencedObj != nullptr)
            {
                PrintSequenceVariable(referencedObj, ss);
            }
            else
            {
                ss << L"SeqVar_External couldn't resolve variable: ";
                ss << static_cast<USeqVar_External*>(seqVar)->VariableLabel.Chars();
            }
        }
        else if (seqVar->IsA(USeqVar_String::StaticClass()))
        {
            ss << static_cast<USeqVar_String*>(seqVar)->StrValue.Chars() << L" ";
        }
        else if (seqVar->IsA(USeqVar_Float::StaticClass()))
        {
            ss << static_cast<USeqVar_Float*>(seqVar)->FloatValue << L" ";
        }
        else if (seqVar->IsA(USeqVar_Int::StaticClass()))
        {
            ss << static_cast<USeqVar_Int*>(seqVar)->IntValue << L" ";
        }
        else if (seqVar->IsA(USeqVar_Bool::StaticClass()))
        {
            ss << (static_cast<USeqVar_Bool*>(seqVar)->bValue ? L"True" : L"False") << L" ";
        }
        else if (seqVar->IsA(USeqVar_Vector::StaticClass()))
        {
            auto* vector = static_cast<USeqVar_Vector*>(seqVar);
            ss << L"Vector X=" << vector->VectValue.X
               << L" Y=" << vector->VectValue.Y
               << L" Z=" << vector->VectValue.Z << L" ";
        }
        else if (seqVar->IsA(USeqVar_Object::StaticClass()))
        {
            auto* seqVarObj = static_cast<USeqVar_Object*>(seqVar);
            auto* referencedObj = seqVarObj->ObjValue;
            if (referencedObj != nullptr)
            {
                auto objName = referencedObj->GetFullPath();
                ss << objName.Chars() << L" ";
            }
        }
        else if (seqVar->IsA(USeqVar_Name::StaticClass()))
        {
            auto nameValue = static_cast<USeqVar_Name*>(seqVar)->NameValue.ToString();
            ss << nameValue.Chars() << L" ";
        }
        else
        {
            auto className = seqVar->Class->Name.ToString();
            ss << L"Unknown SeqVar type: " << className.Chars();
        }
    }

    // ! UObject::ProcessEvent hook
    // ========================================

    t_UObject_ProcessEvent* UObject_ProcessEvent_orig = nullptr;

    void UObject_ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result)
    {
        auto functionName = Function->GetFullName();

        // Handle SeqAct_Log activation
        if (functionName.Equals(L"Function Engine.SequenceOp.Activated") && Context->IsA(USeqAct_Log::StaticClass()))
        {
            std::wstringstream ss;
            auto* seqLog = static_cast<USeqAct_Log*>(Context);

            // Output object comments if enabled
            if (seqLog->bOutputObjCommentToScreen)
            {
                for (auto& comment : seqLog->m_aObjComment)
                {
                    ss << comment.Chars() << L" ";
                }
            }

            // Output all linked variables
            for (auto& varLink : seqLog->VariableLinks)
            {
                for (auto* seqVar : varLink.LinkedVariables)
                {
                    if (seqVar != nullptr)
                    {
                        PrintSequenceVariable(seqVar, ss);
                    }
                }
            }

            std::wstring msg = ss.str();
            LEASI_TRACE(L"SeqAct_Log: {}", msg.c_str());
            if (!msg.empty())
            {
                if (FileLogger)
                {
                    FileLogger->log(L"{}", msg.c_str());
                    FileLogger->flush();
                }
                if (OnScreenLogger)
                {
                    OnScreenLogger->LogMessage(msg.c_str());
                }
            }
        }
        // Handle BioHUD.PostRender for on-screen display
        else if (functionName.Equals(L"Function SFXGame.BioHUD.PostRender"))
        {
            // Toggle drawing/not drawing
            if ((GetKeyState('G') & 0x8000) && (GetKeyState(VK_CONTROL) & 0x8000)) 
            {
                if (CanToggleDrawLog) {
                    CanToggleDrawLog = false; // Will not activate combo again until you re-press combo
                    DrawLog = !DrawLog;
                }
            }
            else
            {
                if (!(GetKeyState('G') & 0x8000) || !(GetKeyState(VK_CONTROL) & 0x8000)) 
                {
                    CanToggleDrawLog = true; // can press key combo again
                }
            }
            auto* hud = static_cast<ABioHUD*>(Context);
            if (DrawLog && OnScreenLogger)
            {
                OnScreenLogger->PostRenderer(hud);
            }
        }

        UObject_ProcessEvent_orig(Context, Function, Parms, Result);
        return;
    }
}
