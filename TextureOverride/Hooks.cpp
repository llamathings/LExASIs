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
}
