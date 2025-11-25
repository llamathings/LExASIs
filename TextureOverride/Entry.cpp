#include <spdlog/details/windows_include.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "Common/Base.hpp"
#include "TextureOverride/Entry.hpp"
#include "TextureOverride/Hooks.hpp"
#include "TextureOverride/Loading.hpp"

SPI_PLUGINSIDE_SUPPORT(SDK_TARGET_NAME_W L"TextureOverride", L"d00telemental", L"0.1.0", SPI_GAME_SDK_TARGET, SPI_VERSION_ANY);
SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;


SPI_IMPLEMENT_ATTACH
{
    ::LESDK::Initializer Init{ InterfacePtr, SDK_TARGET_NAME_A "TextureOverride" };

    ::TextureOverride::InitializeLogger();
    ::TextureOverride::InitializeGlobals(Init);
    ::TextureOverride::InitializeHooks(Init);
    ::TextureOverride::InitializeArgs();

    LEASI_INFO("hello there, {}!", SDK_TARGET_NAME_A "TextureOverride");
    ::TextureOverride::LoadDlcManifests();

    return true;
}

SPI_IMPLEMENT_DETACH
{
    LEASI_UNUSED(InterfacePtr);
#ifdef _DEBUG
    ::LESDK::TerminateConsole();
#endif
    return true;
}


namespace TextureOverride
{
    void InitializeLogger()
    {
        auto DefaultLogger = spdlog::default_logger();
        DefaultLogger->sinks().clear();

#ifdef _DEBUG
        ::LESDK::InitializeConsole();

        auto ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        ConsoleSink->set_level(spdlog::level::trace);
        ConsoleSink->set_color(spdlog::level::trace, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        ConsoleSink->set_color(spdlog::level::debug, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        ConsoleSink->set_color(spdlog::level::info, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        DefaultLogger->sinks().push_back(std::move(ConsoleSink));
#endif

        auto FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("TextureOverride.log", true);
        FileSink->set_level(spdlog::level::trace);
        DefaultLogger->sinks().push_back(std::move(FileSink));

        DefaultLogger->set_pattern("%^[%H:%M:%S.%e %l] (" SDK_TARGET_NAME_A "TextureOverride) %v%$");
        DefaultLogger->set_level(spdlog::level::info);

#ifdef _DEBUG
        DefaultLogger->set_level(spdlog::level::trace);
#endif

        spdlog::flush_on(spdlog::level::warn);
        spdlog::flush_every(std::chrono::seconds(5));
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
        auto const UGameEngine_Exec_target = Init.ResolveTyped<t_UGameEngine_Exec>(UGAMEENGINE_EXEC_RVA);
        CHECK_RESOLVED(UGameEngine_Exec_target);
        UGameEngine_Exec_orig = (t_UGameEngine_Exec*)Init.InstallHook("UGameEngine::Exec", UGameEngine_Exec_target, UGameEngine_Exec_hook);
        CHECK_RESOLVED(UGameEngine_Exec_orig);

        auto const UTexture2D_Serialize_target = Init.ResolveTyped<t_UTexture2D_Serialize>(UTEXTURE2D_SERIALIZE_RVA);
        CHECK_RESOLVED(UTexture2D_Serialize_target);
        UTexture2D_Serialize_orig = (t_UTexture2D_Serialize*)Init.InstallHook("UTexture2D::Serialize", UTexture2D_Serialize_target, UTexture2D_Serialize_hook);
        CHECK_RESOLVED(UTexture2D_Serialize_orig);

        // Find oodle decompression function.
        OodleDecompress = Init.ResolveTyped<t_OodleDecompress>(OODLE_DECOMPRESS_RVA);

        LEASI_INFO("hooks initialized");
    }

    void InitializeArgs()
    {
        FString CmdArgs{ GetCommandLineW() };
        CmdArgs.Append(L" ");

        if (CmdArgs.Contains(L" -disabletextureoverride ", true))
        {
            g_enableLoadingManifest = false;
            LEASI_WARN(L"texture override disabled via cmd args");
            LEASI_WARN(L"manifests will still be processed");
        }
    }
}
