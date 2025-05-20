#include "Common/Objects.hpp"
#include "ConvoSniffer/Config.hpp"
#include "ConvoSniffer/Hooks.hpp"
#include "ConvoSniffer/Profile.hpp"


namespace ConvoSniffer
{
    // ! UObject hooks.
    // ========================================

    t_UObject_ProcessEvent* UObject_ProcessEvent_orig = nullptr;
    void UObject_ProcessEvent_hook(UObject* const Context, UFunction* const Function, void* const Parms, void* const Result)
    {
        LEASI_UNUSED_4(Context, Function, Parms, Result);
        UObject_ProcessEvent_orig(Context, Function, Parms, Result);

        if (Function == nullptr)
        {
            LEASI_WARN(L"ProcessEvent: null function");
            return;
        }

#if defined(CNVSNF_LOG_MANAGED_FUNCS) && CNVSNF_LOG_MANAGED_FUNCS
        {
            FString const FunctionName = Function->GetFullName();
            if (FunctionName.Contains(L"BioConversation"))
                LEASI_INFO(L"ProcessEvent: {}", *FunctionName);
        }
#endif

#if defined(CNVSNF_PROFILE_CONVO) && CNVSNF_PROFILE_CONVO
        do {
            ABioHUD* const BioHUD = Context->Cast<ABioHUD>();
            if (BioHUD != nullptr && Function->GetName().Equals(L"PostRender"))
            {
                UEngine* const Engine = Common::FindFirstObject<UEngine, true>();
                if (Engine == nullptr) break;

                AWorldInfo* const WorldInfo = Engine->GetCurrentWorldInfo();
                if (WorldInfo == nullptr) break;

                ABioWorldInfo* const BioWorldInfo = WorldInfo->Cast<ABioWorldInfo>();
                if (BioWorldInfo == nullptr) break;

                {
                    UCanvas* const Canvas = BioHUD->Canvas;
                    UBioConversation* const Conversation = BioWorldInfo->m_oCurrentConversation;

                    if (Canvas != nullptr && Conversation != nullptr)
                        ProfileConversation(Canvas, Conversation);
                }
            }
        } while (false);
#endif
    }

    t_UObject_ProcessInternal* UObject_ProcessInternal_orig = nullptr;
    void UObject_ProcessInternal_hook(UObject* const Context, FFrame* const Stack, void* const Result)
    {
        LEASI_UNUSED_3(Context, Stack, Result);
        UObject_ProcessInternal_orig(Context, Stack, Result);

#if defined(CNVSNF_LOG_MANAGED_FUNCS) && CNVSNF_LOG_MANAGED_FUNCS

        if (Stack->Node == nullptr)
        {
            LEASI_WARN(L"ProcessInternal: null function");
            return;
        }

        FString const FunctionName = Stack->Node->GetFullName();
        if (FunctionName.Contains(L"BioConversation"))
            LEASI_INFO(L"ProcessInternal: {}", *FunctionName);

#endif
    }

    t_UObject_CallFunction* UObject_CallFunction_orig = nullptr;
    void UObject_CallFunction_hook(UObject* const Context, FFrame* const Stack, void* const Result, UFunction* const Function)
    {
        LEASI_UNUSED_4(Context, Stack, Result, Function);
        UObject_CallFunction_orig(Context, Stack, Result, Function);

#if defined(CNVSNF_LOG_MANAGED_FUNCS) && CNVSNF_LOG_MANAGED_FUNCS

        if (Function == nullptr)
        {
            LEASI_WARN(L"CallFunction: null function");
            return;
        }

        FString const FunctionName = Function->GetFullName();
        if (FunctionName.Contains(L"BioConversation") &&
            !FunctionName.Contains(L"UpdateConversation") &&
            !FunctionName.Contains(L"NeedToDisplayReplies") &&
            !FunctionName.Contains(L"SkipNode"))
        {
            LEASI_INFO(L"CallFunction: {}", *FunctionName);
        }

#endif

        UBioConversation* const Conversation = Context->Cast<UBioConversation>();
        if (Conversation != nullptr && Stack->Node != nullptr && Stack->Node->GetName().Equals(L"UpdateConversation"))
        {
            //LEASI_BREAK_SAFE();
            // ...
        }
    }


    // ! UBioConversation hooks.
    // ========================================

    t_UBioConversation_StartConversation* UBioConversation_StartConversation_orig = nullptr;
    UBOOL UBioConversation_StartConversation_hook(UBioConversation* const Context, AActor* const Owner, AActor* const Player)
    {
        LEASI_UNUSED_3(Context, Owner, Player);
        UBOOL const Result = UBioConversation_StartConversation_orig(Context, Owner, Player);

#if defined(CNVSNF_LOG_NATIVE_FUNCS) && CNVSNF_LOG_NATIVE_FUNCS

        LEASI_INFO(L"StartConversation: Owner = {}, Player = {}",
            Owner ? *Owner->GetName() : L"<n/a>",
            Player ? *Player->GetName() : L"<n/a>");

#endif

        return Result;
    }

    t_UBioConversation_EndConversation* UBioConversation_EndConversation_orig = nullptr;
    void UBioConversation_EndConversation_hook(UBioConversation* const Context)
    {
        LEASI_UNUSED(Context);
        UBioConversation_EndConversation_orig(Context);

#if defined(CNVSNF_LOG_NATIVE_FUNCS) && CNVSNF_LOG_NATIVE_FUNCS

        LEASI_INFO(L"EndConversation");

#endif
    }

    t_UBioConversation_SelectReply* UBioConversation_SelectReply_orig = nullptr;
    UBOOL UBioConversation_SelectReply_hook(UBioConversation* const Context, int const Reply)
    {
        LEASI_UNUSED_2(Context, Reply);
        UBOOL const Result = UBioConversation_SelectReply_orig(Context, Reply);

#if defined(CNVSNF_LOG_NATIVE_FUNCS) && CNVSNF_LOG_NATIVE_FUNCS && false

        LEASI_INFO(L"SelectReply: Reply = {}", Reply);

#endif

        return Result;
    }

    t_UBioConversation_QueueReply* UBioConversation_QueueReply_orig = nullptr;
    UBOOL UBioConversation_QueueReply_hook(UBioConversation* const Context, int const Reply)
    {
        LEASI_UNUSED_2(Context, Reply);
        UBOOL const Result = UBioConversation_QueueReply_orig(Context, Reply);

#if defined(CNVSNF_LOG_NATIVE_FUNCS) && CNVSNF_LOG_NATIVE_FUNCS

        LEASI_INFO(L"QueueReply: Reply = {}", Reply);
        LEASI_INFO(L"  paraphrase = {}", *Context->GetReplyParaphraseText(Reply));
        LEASI_INFO(L"  text = {}", *Context->GetReplyText(Reply, TRUE));

#endif

        return Result;
    }
}
