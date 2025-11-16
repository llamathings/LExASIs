#include "Common/Base.hpp"
#include "ConvoSniffer/Client.hpp"
#include "ConvoSniffer/Entry.hpp"
#include "ConvoSniffer/Hooks.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>


#ifndef SDK_TARGET_LE1
    // This is only a proof of concept for now.
    #error ConvoSniffer target only supports LE1 at the moment.
#endif


SPI_PLUGINSIDE_SUPPORT(L"LE1ConvoSniffer", L"d00telemental", L"0.1.0", SPI_GAME_LE1, SPI_VERSION_ANY);
SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;


SPI_IMPLEMENT_ATTACH
{
    ::LESDK::Initializer Init{ InterfacePtr, "LE1ConvoSniffer" };
    ::LESDK::InitializeConsole();

    ::ConvoSniffer::InitializeLogger();

    LEASI_INFO("Launching ConvoSniffer ({})...", "LE1");
    LEASI_INFO("-------------------------------");

    ::ConvoSniffer::InitializeGlobals(Init);
    ::ConvoSniffer::InitializeHooks(Init);

    ::Sleep(3 * 1000);
    ::ConvoSniffer::gp_snifferClient = new ConvoSniffer::SnifferClient("localhost", 21830);

    return true;
}

SPI_IMPLEMENT_DETACH
{
    LEASI_UNUSED(InterfacePtr);
    delete std::exchange(::ConvoSniffer::gp_snifferClient, nullptr);
    ::LESDK::TerminateConsole();
    return true;
}


namespace ConvoSniffer
{
    void InitializeLogger()
    {
        auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console->set_pattern("%^[%H:%M:%S.%e] [%L LE1ConvoSniffer] %v%$");
        console->set_level(spdlog::level::debug);

        auto file = std::make_shared<spdlog::sinks::basic_file_sink_mt>("convosniffer.le1.log", false);
        file->set_pattern("[%H:%M:%S.%e] [%l] %v");
        file->set_level(spdlog::level::trace);

        auto logger = new spdlog::logger("multi_sink", { console, file });
        logger->set_level(spdlog::level::trace);
        spdlog::set_default_logger(std::shared_ptr<spdlog::logger>(logger));
    }

#define CHECK_RESOLVED(variable)                                                    \
    do {                                                                            \
        LEASI_VERIFYA(variable != nullptr, "failed to resolve " #variable, "");     \
        LEASI_TRACE("resolved " #variable " => {}", (void*)variable);               \
    } while (false)

    void InitializeGlobals(::LESDK::Initializer& Init)
    {
        GMalloc = Init.ResolveTyped<FMallocLike*>(BUILTIN_GMALLOC_RIP);
        CHECK_RESOLVED(GMalloc);

        //GEngine = Init.ResolveTyped<UEngine*>(BUILTIN_GENGINE_RIP);
        //CHECK_RESOLVED(GEngine);
        //GNatives = Init.ResolveTyped<tNative*>(BUILTIN_GNATIVES_RIP);
        //CHECK_RESOLVED(GNatives);
        //GSys = Init.ResolveTyped<USystem*>(BUILTIN_GSYS_RIP);
        //CHECK_RESOLVED(GSys);
        //GWorld = Init.ResolveTyped<UWorld*>(BUILTIN_GWORLD_RIP);
        //CHECK_RESOLVED(GWorld);

        UObject::GObjObjects = Init.ResolveTyped<TArray<UObject*>>(BUILTIN_GOBOBJECTS_RIP);
        CHECK_RESOLVED(UObject::GObjObjects);
        SFXName::GBioNamePools = Init.ResolveTyped<SFXNameEntry const*>(BUILTIN_SFXNAMEPOOLS_RIP);
        CHECK_RESOLVED(SFXName::GBioNamePools);
        SFXName::GInitMethod = Init.ResolveTyped<SFXName::tInitMethod>(BUILTIN_SFXNAMEINIT_PHOOK);
        CHECK_RESOLVED(SFXName::GInitMethod);

        LEASI_INFO("globals initialized");
    }

    void InitializeHooks(::LESDK::Initializer& Init)
    {
        // UObject hooks.
        // ----------------------------------------

        {
            auto const UObject_ProcessEvent_target = Init.ResolveTyped<t_UObject_ProcessEvent>(BUILTIN_PROCESSEVENT_PHOOK);
            CHECK_RESOLVED(UObject_ProcessEvent_target);
            UObject_ProcessEvent_orig = (t_UObject_ProcessEvent*)Init.InstallHook("UObject::ProcessEvent", UObject_ProcessEvent_target, UObject_ProcessEvent_hook);
            CHECK_RESOLVED(UObject_ProcessEvent_orig);
        }

        {
            auto const UObject_ProcessInternal_target = Init.ResolveTyped<t_UObject_ProcessInternal>(BUILTIN_PROCESSINTERNAL_PHOOK);
            CHECK_RESOLVED(UObject_ProcessInternal_target);
            UObject_ProcessInternal_orig = (t_UObject_ProcessInternal*)Init.InstallHook("UObject::ProcessInternal", UObject_ProcessInternal_target, UObject_ProcessInternal_hook);
            CHECK_RESOLVED(UObject_ProcessInternal_orig);
        }

        {
            auto const UObject_CallFunction_target = Init.ResolveTyped<t_UObject_CallFunction>(BUILTIN_CALLFUNCTION_PHOOK);
            CHECK_RESOLVED(UObject_CallFunction_target);
            UObject_CallFunction_orig = (t_UObject_CallFunction*)Init.InstallHook("UObject::CallFunction", UObject_CallFunction_target, UObject_CallFunction_hook);
            CHECK_RESOLVED(UObject_CallFunction_orig);
        }

        LEASI_INFO("UObject hooks initialized");


        // UGameEngine hooks.
        // ----------------------------------------

        {
            auto const UGameEngine_Exec_target = Init.ResolveTyped<t_UGameEngine_Exec>(CONVO_SNIFFER_EXEC_PHOOK);
            CHECK_RESOLVED(UGameEngine_Exec_target);
            UGameEngine_Exec_orig = (t_UGameEngine_Exec*)Init.InstallHook("UGameEngine::Exec", UGameEngine_Exec_target, UGameEngine_Exec_hook);
            CHECK_RESOLVED(UGameEngine_Exec_orig);
        }

        LEASI_INFO("UGameEngine hooks initialized");


        // UGameViewportClient hooks.
        // ----------------------------------------

        {
            auto const UGameViewportClient_InputKey_target = Init.ResolveTyped<t_UGameViewportClient_InputKey>(CONVO_SNIFFER_INPUTKEY_PAT);
            CHECK_RESOLVED(UGameViewportClient_InputKey_target);
            UGameViewportClient_InputKey_orig = (t_UGameViewportClient_InputKey*)Init.InstallHook("UGameViewportClient::InputKey", UGameViewportClient_InputKey_target, UGameViewportClient_InputKey_hook);
            CHECK_RESOLVED(UGameViewportClient_InputKey_orig);
        }

        LEASI_INFO("UGameViewportClient hooks initialized");


        // UBioConversation hooks.
        // ----------------------------------------

        {
            auto const UBioConversation_StartConversation_target = Init.ResolveTyped<t_UBioConversation_StartConversation>(CONVO_SNIFFER_STARTCONVERSATION_PAT);
            CHECK_RESOLVED(UBioConversation_StartConversation_target);
            UBioConversation_StartConversation_orig = (t_UBioConversation_StartConversation*)Init.InstallHook("UBioConversation::StartConversation", UBioConversation_StartConversation_target, UBioConversation_StartConversation_hook);
            CHECK_RESOLVED(UBioConversation_StartConversation_orig);
        }

        {
            auto const UBioConversation_EndConversation_target = Init.ResolveTyped<t_UBioConversation_EndConversation>(CONVO_SNIFFER_ENDCONVERSATION_PAT);
            CHECK_RESOLVED(UBioConversation_EndConversation_target);
            UBioConversation_EndConversation_orig = (t_UBioConversation_EndConversation*)Init.InstallHook("UBioConversation::EndConversation", UBioConversation_EndConversation_target, UBioConversation_EndConversation_hook);
            CHECK_RESOLVED(UBioConversation_EndConversation_orig);
        }

        {
            auto const UBioConversation_SelectReply_target = Init.ResolveTyped<t_UBioConversation_SelectReply>(CONVO_SNIFFER_SELECTREPLY_PAT);
            CHECK_RESOLVED(UBioConversation_SelectReply_target);
            UBioConversation_SelectReply_orig = (t_UBioConversation_SelectReply*)Init.InstallHook("UBioConversation::SelectReply", UBioConversation_SelectReply_target, UBioConversation_SelectReply_hook);
            CHECK_RESOLVED(UBioConversation_SelectReply_orig);
        }

        {
            auto const UBioConversation_QueueReply_target = Init.ResolveTyped<t_UBioConversation_QueueReply>(CONVO_SNIFFER_QUEUEREPLY_PAT);
            CHECK_RESOLVED(UBioConversation_QueueReply_target);
            UBioConversation_QueueReply_orig = (t_UBioConversation_QueueReply*)Init.InstallHook("UBioConversation::QueueReply", UBioConversation_QueueReply_target, UBioConversation_QueueReply_hook);
            CHECK_RESOLVED(UBioConversation_QueueReply_orig);
        }

        LEASI_INFO("UBioConversation hooks initialized");
    }

    void DumpConvoFunctions()
    {
        SFXName const NAME_BioConversation(L"BioConversation", 0);
        LEASI_INFO(L"SFXGame.BioConversation methods:");

        for (UObject* const Object : *UObject::GObjObjects)
        {
            UFunction* const Function = Object ? Object->Cast<UFunction>() : nullptr;
            if (Function == nullptr || Function->Outer == nullptr) continue;

            if (Function->Outer->Name == NAME_BioConversation)
            {
                auto const Absolute = reinterpret_cast<BYTE*>(Function->Func);
                auto const Relative = Absolute - reinterpret_cast<BYTE*>(LESDK::GetMainModuleBase());
                LEASI_INFO(L" {} => BASE + 0x{:x} = {:p}", *Function->GetFullName(), Relative, (void*)Absolute);
            }
        }
    }
}
