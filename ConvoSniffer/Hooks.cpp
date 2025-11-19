#include "Common/Objects.hpp"
#include "ConvoSniffer/Client.hpp"
#include "ConvoSniffer/Config.hpp"
#include "ConvoSniffer/Client.hpp"
#include "ConvoSniffer/Hooks.hpp"
#include "ConvoSniffer/Profile.hpp"


namespace ConvoSniffer
{
    // ! UObject hooks.
    // ========================================

    t_UObject_ProcessEvent* UObject_ProcessEvent_orig = nullptr;
    void UObject_ProcessEvent_hook(UObject* const Context, UFunction* const Function, void* const Parms, void* const Result)
    {
        UObject_ProcessEvent_orig(Context, Function, Parms, Result);

#if defined(CNVSNF_LOG_MANAGED_FUNCS) && CNVSNF_LOG_MANAGED_FUNCS
        {
            FString const FunctionName = Function->GetFullName();
            if (FunctionName.Contains(L"BioConversation"))
                LEASI_INFO(L"ProcessEvent: {}", *FunctionName);
        }
#endif

#if defined(CNVSNF_PROFILE_CONVO) && CNVSNF_PROFILE_CONVO
        if (gb_renderProfile)
        {
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
        }
#endif
    }

    t_UObject_ProcessInternal* UObject_ProcessInternal_orig = nullptr;
    void UObject_ProcessInternal_hook(UObject* const Context, FFrame* const Stack, void* const Result)
    {
        UObject_ProcessInternal_orig(Context, Stack, Result);

#if defined(CNVSNF_LOG_MANAGED_FUNCS) && CNVSNF_LOG_MANAGED_FUNCS
        {
            FString const FunctionName = Stack->Node->GetFullName();
            if (FunctionName.Contains(L"BioConversation"))
                LEASI_INFO(L"ProcessInternal: {}", *FunctionName);
        }
#endif
    }

    t_UObject_CallFunction* UObject_CallFunction_orig = nullptr;
    void UObject_CallFunction_hook(UObject* const Context, FFrame* const Stack, void* const Result, UFunction* const Function)
    {
        UObject_CallFunction_orig(Context, Stack, Result, Function);

#if defined(CNVSNF_LOG_MANAGED_FUNCS) && CNVSNF_LOG_MANAGED_FUNCS
        {
            FString const FunctionName = Function->GetFullName();
            if (FunctionName.Contains(L"BioConversation"))
                LEASI_INFO(L"CallFunction: {}", *FunctionName);
        }
#endif

        bool bUpdateConversationHandled = false;

        UBioConversation* const Conversation = Context->CastDirect<UBioConversation>();
        if (Conversation != nullptr && Stack->Node != nullptr && gp_snifferClient != nullptr)
        {
            FString const Name = Stack->Node->GetName();
            if (Name.Equals(L"UpdateConversation"))
            {
                gp_snifferClient->OnUpdateConversation(Conversation);
                bUpdateConversationHandled = true;
            }
        }

        if (!bUpdateConversationHandled)
        {
            gp_snifferClient->OnVeryFrequentUpdateConversation();
            bUpdateConversationHandled = true;
        }
    }


    // ! UGameEngine hooks.
    // ========================================

    t_UGameEngine_Exec* UGameEngine_Exec_orig = nullptr;
    DWORD UGameEngine_Exec_hook(UGameEngine* const Context, WCHAR const* const Command, void* const Archive)
    {
        static bool sb_commandsLogged = false;
        if (!std::exchange(sb_commandsLogged, true))
        {
            LEASI_INFO(L"Invoke 'show scaleform' to toggle UI manually, if needed.");
            LEASI_INFO(L"Available extra commands:");
            LEASI_INFO(L" - cs.profile - toggles profiler rendering (in convos)");
            LEASI_INFO(L" - cs.hud - toggles scaleform rendering (in convos)");
        }

        if (Command != nullptr)
        {
            FString const Cmd{ Command };
            if (Cmd.StartsWith(L"cs.", true))
            {
                if (Cmd.Equals(L"cs.profile", true))
                {
                    LEASI_INFO(L"Toggle profiler rendering.");
                    gb_renderProfile = !gb_renderProfile;
                }
                else if (Cmd.Equals(L"cs.hud", true))
                {
                    LEASI_INFO(L"Toggle user interface.");
                    gb_renderScaleform = !gb_renderScaleform;
                }
            }
        }

        return UGameEngine_Exec_orig(Context, Command, Archive);
    }


    // ! UGameViewportClient hooks.
    // ========================================

    t_UGameViewportClient_InputKey* UGameViewportClient_InputKey_orig = nullptr;
    DWORD UGameViewportClient_InputKey_hook
    (
        UGameViewportClient* const      Context,
        void* const                     Viewport,
        DWORD const                     Unknown0,
        SFXName const                   Key,
        int const                       EventType,
        DWORD const                     Unknown1,
        DWORD const                     Unknown2
    )
    {
        if (gp_snifferClient && gp_snifferClient->InConversation())
        {
            // Only interested in "released" key events.
            if (EventType == 1)
            {
                FString const   KeyStr = Key.ToString();
                bool            bKeyHandled = false;

                LEASI_DEBUG(L"UGameViewportClient::InputKey => {} (released)", *KeyStr);

                if (false) {}
                else if (KeyStr.Equals(L"Zero")) bKeyHandled = gp_snifferClient->QueueReplyMapped(0);
                else if (KeyStr.Equals(L"One")) bKeyHandled = gp_snifferClient->QueueReplyMapped(1);
                else if (KeyStr.Equals(L"Two")) bKeyHandled = gp_snifferClient->QueueReplyMapped(2);
                else if (KeyStr.Equals(L"Three")) bKeyHandled = gp_snifferClient->QueueReplyMapped(3);
                else if (KeyStr.Equals(L"Four")) bKeyHandled = gp_snifferClient->QueueReplyMapped(4);
                else if (KeyStr.Equals(L"Five")) bKeyHandled = gp_snifferClient->QueueReplyMapped(5);
                else if (KeyStr.Equals(L"Six")) bKeyHandled = gp_snifferClient->QueueReplyMapped(6);
                else if (KeyStr.Equals(L"Seven")) bKeyHandled = gp_snifferClient->QueueReplyMapped(7);
                else if (KeyStr.Equals(L"Eight")) bKeyHandled = gp_snifferClient->QueueReplyMapped(8);
                else if (KeyStr.Equals(L"Nine")) bKeyHandled = gp_snifferClient->QueueReplyMapped(9);
                else if (KeyStr.Equals(L"A")) bKeyHandled = gp_snifferClient->QueueReplyMapped(10);
                else if (KeyStr.Equals(L"B")) bKeyHandled = gp_snifferClient->QueueReplyMapped(11);
                else if (KeyStr.Equals(L"C")) bKeyHandled = gp_snifferClient->QueueReplyMapped(12);
                else if (KeyStr.Equals(L"D")) bKeyHandled = gp_snifferClient->QueueReplyMapped(13);
                else if (KeyStr.Equals(L"E")) bKeyHandled = gp_snifferClient->QueueReplyMapped(14);
                else if (KeyStr.Equals(L"F")) bKeyHandled = gp_snifferClient->QueueReplyMapped(15);
                
                else if (!gb_renderScaleform && KeyStr.Equals(L"SpaceBar"))
                {
                    // Special case: replace the skip-node binding disabled when GFx is hidden.
                    gp_snifferClient->GetActiveConversation()->SkipNode();
                }

                // Allow the engine to handle 0-9, A-F keys even if we're using them...
                LEASI_UNUSED(bKeyHandled);
            }
        }

        return UGameViewportClient_InputKey_orig(Context, Viewport, Unknown0, Key, EventType, Unknown1, Unknown2);
    }


    // ! UBioConversation hooks.
    // ========================================

    t_UBioConversation_StartConversation* UBioConversation_StartConversation_orig = nullptr;
    UBOOL UBioConversation_StartConversation_hook(UBioConversation* const Context, AActor* const Owner, AActor* const Player)
    {
        UBOOL const Result = UBioConversation_StartConversation_orig(Context, Owner, Player);

#if defined(CNVSNF_LOG_NATIVE_FUNCS) && CNVSNF_LOG_NATIVE_FUNCS

        LEASI_INFO(L"StartConversation: Owner = {}, Player = {}",
            Owner ? *Owner->GetName() : L"<n/a>",
            Player ? *Player->GetName() : L"<n/a>");

#endif

        if (gp_snifferClient)
            gp_snifferClient->OnStartConversation(Context);

        return Result;
    }

    t_UBioConversation_EndConversation* UBioConversation_EndConversation_orig = nullptr;
    void UBioConversation_EndConversation_hook(UBioConversation* const Context)
    {
        if (gp_snifferClient)
            gp_snifferClient->OnEndConversation(Context);

        UBioConversation_EndConversation_orig(Context);

#if defined(CNVSNF_LOG_NATIVE_FUNCS) && CNVSNF_LOG_NATIVE_FUNCS
        LEASI_INFO(L"EndConversation");
#endif
    }

    t_UBioConversation_SelectReply* UBioConversation_SelectReply_orig = nullptr;
    UBOOL UBioConversation_SelectReply_hook(UBioConversation* const Context, int const Reply)
    {
        UBOOL const Result = UBioConversation_SelectReply_orig(Context, Reply);

#if defined(CNVSNF_LOG_NATIVE_FUNCS) && CNVSNF_LOG_NATIVE_FUNCS
        LEASI_INFO(L"SelectReply: Reply = {}", Reply);
#endif

        return Result;
    }

    t_UBioConversation_QueueReply* UBioConversation_QueueReply_orig = nullptr;
    UBOOL UBioConversation_QueueReply_hook(UBioConversation* const Context, int const Reply)
    {
        if (gp_snifferClient)
            gp_snifferClient->OnQueueReply(Context, Reply);

        UBOOL const Result = UBioConversation_QueueReply_orig(Context, Reply);

#if defined(CNVSNF_LOG_NATIVE_FUNCS) && CNVSNF_LOG_NATIVE_FUNCS

        LEASI_INFO(L"QueueReply: Reply = {} (=> {})", Reply, Context->m_lstCurrentReplyIndices(Reply));
        LEASI_INFO(L"  paraphrase = {}", *Context->GetReplyParaphraseText(Reply));
        LEASI_INFO(L"  text = {}", *Context->GetReplyText(Reply, TRUE));

#endif

        return Result;
    }
}
