#include "TextureOverride/Hooks.hpp"
#include "TextureOverride/Loading.hpp"

namespace TextureOverride
{

    t_UGameEngine_Exec* UGameEngine_Exec_orig = nullptr;
    DWORD UGameEngine_Exec_hook(UGameEngine* const Context, WCHAR const* const Command, void* const Archive)
    {
        if (std::wstring_view const CommandView{ Command };
            CommandView.starts_with(L"to.") || CommandView.starts_with(L"TO."))
        {
            FString const CommandCopy{ Command };
            if (CommandCopy.Contains(L"to.disable"))
            {
                g_enableLoadingManifest = false;
                LEASI_WARN(L"texture override disabled via console command");
                LEASI_WARN(L"manifests will still be processed");
            }
            else if (CommandCopy.Contains(L"to.enable"))
            {
                g_enableLoadingManifest = true;
                LEASI_WARN(L"texture override re-enabled via console command");
            }
#ifdef _DEBUG
            else if (CommandCopy.Contains(L"to.stats"))
            {
                g_statTextureSerializeCount = std::max<int>(g_statTextureSerializeCount, 1);

                LEASI_INFO(L"(stats) UTexture2D::Serialize time added:     {:.4f} ms", g_statTextureSerializeSeconds * 1000);
                LEASI_INFO(L"(stats) UTexture2D::Serialize active count:   {}", g_statTextureSerializeCount);
                LEASI_INFO(L"(stats) ==================================================");
                LEASI_INFO(L"(stats) AVERAGE TIME ADDED:                   {:.4f} ms", g_statTextureSerializeSeconds * 1000 / g_statTextureSerializeCount);
            }
#endif
        }

        return UGameEngine_Exec_orig(Context, Command, Archive);
    }

    t_UTexture2D_Serialize* UTexture2D_Serialize_orig = nullptr;
    void UTexture2D_Serialize_hook(UTexture2D* const Context, void* const Archive)
    {
        FString const& TextureFullName = GetTextureFullName(Context);
        //LEASI_TRACE(L"UTexture2D::Serialize: {}", *TextureFullName);

        (*UTexture2D_Serialize_orig)(Context, Archive);

        if (g_enableLoadingManifest)
        {
#ifdef _DEBUG
            ScopedTimer Timer{};
#endif

            for (ManifestLoaderPointer const& Manifest : g_loadedManifests)
            {
                CTextureEntry const* const Entry = Manifest->FindEntry(TextureFullName);
                if (Entry == nullptr) continue;
                LEASI_INFO(L"UTexture2D::Serialize: replacing {}", *TextureFullName);
                UpdateTextureFromManifest(Context, *Manifest, *Entry);
                return;  // apply only the highest-priority manifest mount
            }

#ifdef _DEBUG
            g_statTextureSerializeCount++;
            g_statTextureSerializeSeconds += Timer.GetSeconds();
#endif
        }

        // Clown mode
        // Context->InternalFormatLODBias = 12;
    }

    t_OodleDecompress* OodleDecompress = nullptr;


#if defined(SDK_TARGET_LE3)
    tRegisterTFC* RegisterTFC = nullptr;
    tGetDLCName* GetDLCName = nullptr;
    tRegisterDLCTFC* RegisterDLCTFC_orig = nullptr;

    // List of already mounted DLC names, so we don't mount twice.
    std::set<std::wstring> MountedDLCNames{};
    unsigned long long RegisterDLCTFC_hook(SFXAdditionalContent* content)
    {
        // Run the original DLC TFC registration method
        auto res = RegisterDLCTFC_orig(content);

        // Get name of DLC
        FString dlcName;
        GetDLCName(content, &dlcName);

        if (dlcName.Length() <= 8 || wcsncmp(dlcName.Chars(), L"DLC_MOD_", 8) != 0)
            return res; // Not a DLC mod

        auto dlcNameStr = std::wstring(dlcName.Chars());
        if (MountedDLCNames.find(dlcNameStr) != MountedDLCNames.end())
        {
            return res; // The DLC TFCs were already mounted. Game seems to call this twice for some reason.
        }

        MountedDLCNames.insert(dlcNameStr);

        std::filesystem::path cookedPathStr = content->RelativeDLCPath.Chars();
        auto dlcCookedPath = cookedPathStr.append(L"CookedPCConsole");

        if (std::filesystem::exists(dlcCookedPath))
        {
            std::wstring defaultPath = dlcCookedPath;
            defaultPath += L"\\Textures_";
            defaultPath += dlcName.Chars();
            defaultPath += L".tfc";

            // Find TFCs in CookedPCConsole folder
            std::wstring ext(L".tfc");

            for (auto& p : std::filesystem::recursive_directory_iterator(dlcCookedPath))
            {
                if (p.path().extension() == ext)
                {
                    auto lpath = p.path();
                    auto rpath = defaultPath;
                    if (p.path().compare(defaultPath) != 0) // Not the default TFC
                    {
                        LEASI_INFO(L"Registering additional mod TFC: {}", p.path().c_str());
                        FString tfcName(const_cast<wchar_t*>(p.path().c_str()));
                        RegisterTFC(&tfcName);
                    }
                }
            }
        }

        return res;
    }

#endif
}
