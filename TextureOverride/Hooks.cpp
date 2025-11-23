#include "TextureOverride/Hooks.hpp"
#include "TextureOverride/Loading.hpp"

namespace TextureOverride
{
    t_UTexture2D_Serialize* UTexture2D_Serialize_orig = nullptr;
    void UTexture2D_Serialize_hook(UTexture2D* const Context, void* const Archive)
    {
        FString const& TextureFullName = GetTextureFullName(Context);
        //LEASI_TRACE(L"UTexture2D::Serialize: {}", *TextureFullName);

        (*UTexture2D_Serialize_orig)(Context, Archive);

        if (g_enableLoadingManifest)
        {
            for (ManifestLoaderPointer const& Manifest : g_loadedManifests)
            {
                CTextureEntry const* const Entry = Manifest->FindEntry(TextureFullName);
                if (Entry == nullptr) continue;

                LEASI_INFO(L"UTexture2D::Serialize: replacing {}", *TextureFullName);
                UpdateTextureFromManifest(Context, *Manifest, *Entry);
            }
        }
    }
}
