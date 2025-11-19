#include "TextureOverride/Manifest.hpp"

namespace TextureOverride
{

    // ! Utilities.
    // ========================================

    template<std::size_t Length>
    char const* FindChar(const char(&Haystack)[Length], char const Needle)
    {
        for (std::size_t i = 0; i < Length; ++i)
        {
            if (Haystack[i] == Needle)
                return &(Haystack[i]);
        }
        return nullptr;
    }

    template<std::size_t Length>
    wchar_t const* FindChar(const wchar_t(&Haystack)[Length], wchar_t const Needle)
    {
        for (std::size_t i = 0; i < Length; ++i)
        {
            if (Haystack[i] == Needle)
                return &(Haystack[i]);
        }
        return nullptr;
    }


    // ! Plain-old-data struct implementations.
    // ========================================

    bool CMipEntry::IsEmpty() const noexcept
    {
        return !IsExternal() && UncompressedSize == 0
            && CompressedSize == UINT32_MAX && CompressedOffset == UINT32_MAX;
    }

    std::pair<std::uint16_t, std::uint16_t> CMipEntry::GetDimensions() const noexcept
    {
        return std::make_pair(Width, Height);
    }

    FString CTextureEntry::GetFullPath() const
    {
        LEASI_VERIFYA(FindChar(FullPath, L'\0') != nullptr, "invalid full path", "");
        return FString::Printf(L"%s", FullPath);
    }

    FString CTextureEntry::GetTfcName() const
    {
        LEASI_VERIFYA(FindChar(TfcName, L'\0') != nullptr, "invalid tfc name", "");
        return FString::Printf(L"%s", TfcName);
    }


    // ! ManifestLoader implementation.
    // ========================================

    ManifestLoader::~ManifestLoader()
    {
        if (FileHandle != INVALID_HANDLE_VALUE || MappingHandle || View)
        {
            if (0 == UnmapViewOfFile(View))
            {
                auto const Error = FString::Printf(L"failed to unmap manifest view, error = %d", ::GetLastError());
                OutputDebugStringW(*Error);
                LEASI_WARN(L"{}", *Error);
            }

            if (0 == CloseHandle(MappingHandle))
            {
                auto const Error = FString::Printf(L"failed to close manifest mapping, error = %d", ::GetLastError());
                OutputDebugStringW(*Error);
                LEASI_WARN(L"{}", *Error);
            }

            if (0 == CloseHandle(FileHandle))
            {
                auto const Error = FString::Printf(L"failed to close manifest file, error = %d", ::GetLastError());
                OutputDebugStringW(*Error);
                LEASI_WARN(L"{}", *Error);
            }

            View = NULL;
            MappingHandle = NULL;
            FileHandle = INVALID_HANDLE_VALUE;
        }
    }

    bool ManifestLoader::Load(std::wstring_view const InPath, FString& OutError)
    {
        LEASI_CHECKA(!InPath.empty(), "empty input path", "");

        FileHandle = ::CreateFileW(InPath.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0u, NULL);
        if (FileHandle == INVALID_HANDLE_VALUE)
        {
            OutError = FString::Printf(L"failed to open file, error = %d", ::GetLastError());
            return false;
        }

        MappingHandle = ::CreateFileMappingW(FileHandle, NULL, PAGE_READONLY, 0u, 0u, NULL);
        if (MappingHandle == NULL)
        {
            ::CloseHandle(std::exchange<HANDLE>(FileHandle, reinterpret_cast<HANDLE>(INVALID_HANDLE_VALUE)));
            OutError = FString::Printf(L"failed to create file mapping, error = %d", ::GetLastError());
            return false;
        }

        View = ::MapViewOfFile(MappingHandle, FILE_MAP_READ, 0u, 0u, 0u);
        if (View == NULL)
        {
            ::CloseHandle(std::exchange(MappingHandle, reinterpret_cast<HANDLE>(NULL)));
            ::CloseHandle(std::exchange(FileHandle, reinterpret_cast<HANDLE>(INVALID_HANDLE_VALUE)));
            OutError = FString::Printf(L"failed to map view of file, error = %d", ::GetLastError());
            return false;
        }

#define CLOSE_ERROR(...)                                                                        \
    LEASI_WARN(__VA_ARGS__);                                                                    \
    ::UnmapViewOfFile(std::exchange(View, reinterpret_cast<LPVOID>(NULL)));                     \
    ::CloseHandle(std::exchange(MappingHandle, reinterpret_cast<HANDLE>(NULL)));                \
    ::CloseHandle(std::exchange(FileHandle, reinterpret_cast<HANDLE>(INVALID_HANDLE_VALUE)));

        LARGE_INTEGER OutFileSize{};
        if (0 == GetFileSizeEx(FileHandle, &OutFileSize))
        {
            CLOSE_ERROR("failed to cache manifest file size");
            return false;
        }

        CachedSize = OutFileSize.QuadPart;
        if (CachedSize < sizeof(CManifestHeader))
        {
            CLOSE_ERROR("manifest file too small ({}) for header ({})", CachedSize, sizeof(CManifestHeader));
            return false;
        }

        auto const Header = (CManifestHeader const*)View;
        if (0 != std::memcmp(Header->Magic, CManifestHeader::k_checkMagic, sizeof(CManifestHeader::k_checkMagic))
            || Header->Version > CManifestHeader::k_lastVersion)
        {
            CLOSE_ERROR("manifest file has invalid magic or version");
            return false;
        }

        auto const EntryTableEnd = sizeof(CManifestHeader) + sizeof(CTextureEntry) * Header->TextureCount;
        if (CachedSize < EntryTableEnd)
        {
            CLOSE_ERROR("entry table end ({}) out of manifest file bounds ({})", EntryTableEnd, CachedSize);
            return false;
        }

        // TODO: This is an implicit requirement, we could try to force-align it.
        LEASI_VERIFYW(EntryTableEnd % 4 == 0, L"entry table end not aligned", L"");

        TextureMap.reserve(static_cast<std::size_t>(Header->TextureCount));
        auto const* const TextureEntryStart = (CTextureEntry const*)((unsigned char const*)View + sizeof(CManifestHeader));

        for (std::size_t i = 0; i < Header->TextureCount; ++i)
        {
            CTextureEntry const* const Entry = TextureEntryStart + i;
            FString const EntryFullPath = Entry->GetFullPath();

            LEASI_TRACE(L"adding manifest entry {} with {} mip(s) in texture file cache '{}'",
                *EntryFullPath, Entry->MipCount, *Entry->GetTfcName());

            bool const bUniqueTexture = TextureMap.emplace(EntryFullPath, Entry).second;
            if (!bUniqueTexture)
            {
                // This probably doesn't need to be a fatal error...
                LEASI_WARN(L"manifest entry {} was not unique", *EntryFullPath);
            }
        }

#undef CLOSE_ERROR

        return true;
    }

    CTextureEntry const* ManifestLoader::FindEntry(FString const& FullPath) const
    {
        LEASI_CHECKA(FileHandle != INVALID_HANDLE_VALUE, "file not loaded", "");
        LEASI_CHECKA(MappingHandle != NULL, "mapping not loaded", "");
        LEASI_CHECKA(View != NULL, "view not loaded", "");

        auto const Iter = TextureMap.find(FullPath);
        if (Iter != TextureMap.end())
            return Iter->second;

        return nullptr;
    }

    ManifestLoader::ResolvedMip ManifestLoader::GetEntryMip(CTextureEntry const& InEntry, std::size_t const Index) const
    {
        LEASI_CHECKA(ValidateEntry(InEntry), "mismatched entry provenance", "");
        LEASI_CHECKA(Index < InEntry.MipCount, "mip index ({}) out of bounds ({})", Index, InEntry.MipCount);

        ResolvedMip Resolved{};
        Resolved.Entry = InEntry.Mips[Index];

        // All mips that are "empty", "original", or "external" must specify no embedded payload.
        if (!Resolved.Entry.IsEmpty() && !Resolved.Entry.IsOriginal() && !Resolved.Entry.IsExternal())
        {
            auto const [Offset, Count] = std::tie(Resolved.Entry.CompressedOffset, Resolved.Entry.CompressedSize);
            Resolved.Payload = GetMappedView().subspan(static_cast<std::size_t>(Offset), static_cast<std::size_t>(Count));
        }

        return Resolved;
    }

    CManifestHeader const& ManifestLoader::GetMappedHeader() const
    {
        LEASI_CHECKA(FileHandle != INVALID_HANDLE_VALUE, "file not loaded", "");
        LEASI_CHECKA(MappingHandle != NULL, "mapping not loaded", "");
        LEASI_CHECKA(View != NULL, "view not loaded", "");

        return *reinterpret_cast<CManifestHeader const*>(View);
    }

    std::span<unsigned char const> ManifestLoader::GetMappedView() const
    {
        LEASI_CHECKA(FileHandle != INVALID_HANDLE_VALUE, "file not loaded", "");
        LEASI_CHECKA(MappingHandle != NULL, "mapping not loaded", "");
        LEASI_CHECKA(View != NULL, "view not loaded", "");

        auto const ViewPointer = reinterpret_cast<unsigned char const*>(View);
        return std::span<unsigned char const>(ViewPointer, CachedSize);
    }

    bool ManifestLoader::Compare(ManifestLoaderPointer const& Left, ManifestLoaderPointer const& Right)
    {
        return Left->MountPriority >= Right->MountPriority;
    }
}
