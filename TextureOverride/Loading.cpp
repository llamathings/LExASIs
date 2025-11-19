#include <filesystem>
#include "TextureOverride/Loading.hpp"
#include "TextureOverride/Manifest.hpp"

namespace fs = std::filesystem;


namespace TextureOverride
{
    std::vector<std::shared_ptr<ManifestLoader>> g_loadedManifests{};

    void LoadDlcManifests()
    {
        fs::path const DlcFolder{ k_searchFoldersRoot };
        LEASI_INFO(L"looking for dlc roots in {}", DlcFolder.c_str());

        for (auto const& DlcRoot : fs::directory_iterator{ DlcFolder })
        {
            auto const& DlcPath = DlcRoot.path();
            std::wstring const DlcName{ DlcPath.filename().c_str() };

            if (!DlcRoot.is_directory() || !DlcName.starts_with(L"DLC_MOD_"))
            {
                //LEASI_TRACE(L"disregarding {}, not a valid directory", DlcPath.c_str());
                continue;
            }

            auto const ManifestPath = DlcPath / "CombinedTextureOverrides.btp";
            LEASI_TRACE(L"looking for manifest {}", ManifestPath.c_str());

            if (!fs::exists(ManifestPath))
                continue;

            FString LoadError{};
            LEASI_DEBUG(L"loading manifest {}", ManifestPath.c_str());

            auto Manifest = std::make_shared<ManifestLoader>();
            if (!Manifest->Load(ManifestPath.wstring(), LoadError))
            {
                LEASI_ERROR(L"failed to load manifest {}", ManifestPath.c_str());
                LEASI_ERROR(L"error: {}", *LoadError);
                continue;
            }

            g_loadedManifests.push_back(std::move(Manifest));
        }
    }

    FString const& GetTextureFullName(UTexture2D* const InObject)
    {
        thread_local FString OutString{};
        OutString.Clear();

        if (InObject->Class != nullptr)
        {
            if (InObject->Outer != nullptr)
            {
                if (InObject->Outer->Outer != nullptr)
                {
                    LESDK::AppendObjectName(InObject->Outer->Outer, OutString, SFXName::k_formatBasic);
                    OutString.Append(L".");
                }

                LESDK::AppendObjectName(InObject->Outer, OutString, SFXName::k_formatBasic);
                OutString.Append(L".");
            }

            LESDK::AppendObjectName(InObject, OutString, SFXName::k_formatInstanced);
            return OutString;
        }

        OutString.Append(L"(null)");
        return OutString;
    }

    void UpdateTextureFromManifest(UTexture2D* const InTexture,
        ManifestLoader const& Manifest, CTextureEntry const& Entry)
    {
        LEASI_CHECKA(InTexture != nullptr, "", "");

        if (Entry.MipCount < 1 || Entry.MipCount > CTextureEntry::k_maxMipCount)
        {
            LEASI_WARN("UpdateTextureFromManifest: aborting due to invalid mip count {}", Entry.MipCount);
            return;
        }

        if (Entry.MipCount != InTexture->Mips.ArrayNum)
        {
            // Idk how much this would break.
            LEASI_WARN("UpdateTextureFromManifest: updating with different mip count, not well-tested");
        }

        // Assuming the first mip is the largest so we can use its size.
        auto const& [FirstEntry, FirstContents] = Manifest.GetEntryMip(Entry, 0);

        InTexture->TextureFileCacheGuid = InTexture->TFCFileGuid = (FGuid)Entry.TfcGuid;
        InTexture->TextureFileCacheName = SFXName(*Entry.GetTfcName(), 0);
        InTexture->OriginalSizeX = InTexture->SizeX = FirstEntry.Width;
        InTexture->OriginalSizeY = InTexture->SizeY = FirstEntry.Height;
        InTexture->Format = (unsigned char)Entry.Format;

        void* PreservedVftable = nullptr;

        // Deallocate the existing indirect mip array.
        {
            auto& Mips = *(TArray<CMipMapInfo*>*) &InTexture->Mips;
            for (CMipMapInfo* Mip : Mips)
            {
                if (PreservedVftable == nullptr)
                {
                    // The first vftable encountered is likely correct.
                    PreservedVftable = Mip->Vftable;
                }
                else if (PreservedVftable != Mip->Vftable)
                {
                    LEASI_WARN("UpdateTextureFromManifest: different vftables encountered: {} != {}",
                        (void*)PreservedVftable, (void*)Mip->Vftable);
                }


                if (Mip->bNeedsFree)
                {
                    (*GMalloc)->Free(Mip->Data);
                    Mip->Data = nullptr;
                }

                (*GMalloc)->Free(Mip);
            }

            Mips.Clear();
            Mips.Shrink();
        }

        // Allocate a new indirect mip array.
        {
            auto& Mips = *new ((void*)&InTexture->Mips) TArray<CMipMapInfo*>();
            for (int i = 0; i < Entry.MipCount; ++i)
            {
                auto const [MipEntry, MipContents] = Manifest.GetEntryMip(Entry, i);

                auto const NextMip = new CMipMapInfo();
                std::memset(NextMip, 0, sizeof *NextMip);

                NextMip->Vftable = PreservedVftable;
                NextMip->Flags = ETF_SingleUse;
                if (MipEntry.IsExternal())
                {
                    auto const ExternalFlags = ETF_External | ETF_OodleCompression;
                    NextMip->Flags = static_cast<ETextureFlags>(NextMip->Flags | ExternalFlags);
                }
                NextMip->Elements = MipEntry.UncompressedSize;
                NextMip->CompressedOffset = MipEntry.CompressedOffset;
                NextMip->CompressedSize = MipEntry.CompressedSize;
                NextMip->Data = nullptr;
                if (MipEntry.ShouldHavePayload())
                {
                    LEASI_CHECKA(!MipContents.empty(), "empty mip payload", "");
                    NextMip->Data = (void*)MipContents.data();
                }
                NextMip->bNeedsFree = FALSE;
                NextMip->Archive = nullptr;
                NextMip->Width = MipEntry.Width;
                NextMip->Height = MipEntry.Height;

                Mips.Add(NextMip);
            }
        }

        // Update InternalFormatLODBias to allow higher than LOD level mips to show without
        // package edits.
		InTexture->InternalFormatLODBias = Entry.InternalFormatLODBias;
    
        // Update NeverStream based on incoming texture entry.
        // Validation of this will come from our editor tools, don't make it our
        // job to handle it.
        InTexture->NeverStream = Entry.NeverStream;
    }
}
