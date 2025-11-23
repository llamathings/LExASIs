#include "TextureOverride/Manifest.hpp"

namespace TextureOverride
{

    // ! Utilities.
    // ========================================

    // Calculates 32-bit FNV-1 hash with per-byte input.
    struct FNV1Hash32 final
    {
        uint32_t Value;
        uint32_t Prime;

        static constexpr uint32_t DEFAULT_OFFSET = 0x811c9dc5;
        static constexpr uint32_t DEFAULT_PRIME = 0x01000193;

        explicit FNV1Hash32(uint32_t const InOffset = DEFAULT_OFFSET, uint32_t const InPrime = DEFAULT_PRIME)
            : Value{ InOffset }, Prime{ InPrime } {}
        explicit operator uint32_t() const { return Value; }

        __forceinline void AddByte(unsigned char const Byte)
        {
            Value = (Value * Prime) ^ Byte;
        }

        FNV1Hash32& operator<<(wchar_t const* String)
        {
            wchar_t Char;

            while ((Char = *String++) != L'\0')
            {
                static_assert(sizeof Char == 2);
                AddByte(static_cast<unsigned char>((Char & 0x00FF) >> 0x00));
                AddByte(static_cast<unsigned char>((Char & 0xFF00) >> 0x08));
            }

            return *this;
        }

        FNV1Hash32& operator<<(std::wstring_view const String)
        {
            for (wchar_t Char : String)
            {
                static_assert(sizeof Char == 2);
                AddByte(static_cast<unsigned char>((Char & 0x00FF) >> 0x00));
                AddByte(static_cast<unsigned char>((Char & 0xFF00) >> 0x08));
            }

            return *this;
        }
    };

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

    bool ManifestLoader::Load
    (
        std::wstring_view const     InPath,
        std::wstring_view const     InDlcName,
        FString&                    OutError
    )
    {
        LEASI_CHECKW(!InPath.empty(), L"empty input path", L"");
        LEASI_CHECKW(!InDlcName.empty(), L"empty input dlc name", L"");
        // This should've been stripped out by the caller:
        LEASI_CHECKW(!InDlcName.starts_with(L"DLC_MOD_"), L"", L"");

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
            CLOSE_ERROR(L"failed to cache manifest file size");
            return false;
        }

        CachedSize = OutFileSize.QuadPart;
        if (CachedSize < sizeof(CManifestHeader))
        {
            CLOSE_ERROR(L"manifest file too small ({}) for header ({})", CachedSize, sizeof(CManifestHeader));
            return false;
        }

        auto const Header = (CManifestHeader const*)View;
        if (0 != std::memcmp(Header->Magic, CManifestHeader::k_checkMagic, sizeof(CManifestHeader::k_checkMagic))
            || Header->Version > CManifestHeader::k_lastVersion)
        {
            CLOSE_ERROR(L"manifest file has invalid magic or version");
            return false;
        }

        // Check folder hash.

        uint32_t const CalculatedHash = (FNV1Hash32{} << SDK_TARGET_NAME_W << InDlcName).Value;
        uint32_t const SerializedHash = Header->TargetHash;

        if (CalculatedHash != SerializedHash)
        {
#ifdef _DEBUG
            LEASI_WARN(L"mismatched serialized folder hash: 0x{:08X}, expected 0x{:08X}",
                SerializedHash, CalculatedHash);
            LEASI_BREAK_SAFE();
#else
            CLOSE_ERROR(L"mismatched serialized folder hash: 0x{:08X}, expected 0x{:08X}",
                SerializedHash, CalculatedHash);
            return false;
#endif
        }

        // Read texture file cache references.

        std::size_t const TfcRefTableOffset = Header->TfcRefOffset;
        std::size_t const TfcRefTableEnd = TfcRefTableOffset + sizeof(CTfcRefEntry) * Header->TfcRefCount;

        if (CachedSize < TfcRefTableEnd)
        {
            CLOSE_ERROR(L"tfc reference table end ({}) out of manifest file bounds ({})", TfcRefTableEnd, CachedSize);
            return false;
        }

        LEASI_VERIFYW(TfcRefTableOffset % 4 == 0, L"tfc ref table not aligned", L"");
        LEASI_VERIFYW(TfcRefTableEnd % 4 == 0, L"tfc ref table end not aligned", L"");

        TfcRefTable = std::span<CTfcRefEntry const>(
            (CTfcRefEntry const*)((unsigned char const*)View + TfcRefTableOffset),
            static_cast<std::size_t>(Header->TfcRefCount)
        );

        // Read texture entries.

        std::size_t const EntryTableOffset = sizeof(CManifestHeader);
        std::size_t const EntryTableEnd = EntryTableOffset + sizeof(CTextureEntry) * Header->TextureCount;

        if (CachedSize < EntryTableEnd)
        {
            CLOSE_ERROR(L"entry table end ({}) out of manifest file bounds ({})", EntryTableEnd, CachedSize);
            return false;
        }

        LEASI_VERIFYW(EntryTableOffset % 4 == 0, L"entry table not aligned", L"");
        LEASI_VERIFYW(EntryTableEnd % 4 == 0, L"entry table end not aligned", L"");

        TextureMap.reserve(static_cast<std::size_t>(Header->TextureCount));
        auto const* const TextureEntryStart = (CTextureEntry const*)((unsigned char const*)View + EntryTableOffset);

        for (std::size_t i = 0; i < Header->TextureCount; ++i)
        {
            CTextureEntry const* const Entry = TextureEntryStart + i;
            FString const EntryFullPath = Entry->GetFullPath();
            FString const TfcName = GetTfcName(Entry);

            if (TfcName == L"None")
            {
                LEASI_TRACE(L"adding manifest entry {} with {} mip(s) (package stored)",
                    *EntryFullPath, Entry->MipCount);
            }
            else
            {
                LEASI_TRACE(L"adding manifest entry {} with {} mip(s) in texture file cache '{}'",
                    *EntryFullPath, Entry->MipCount, *TfcName);
            }
            
            auto [_, bUniqueTexture] = TextureMap.emplace(EntryFullPath, Entry);
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

    FGuid ManifestLoader::GetTfcGuid(CTextureEntry const* const Entry) const
    {
        LEASI_CHECKA(Entry != nullptr, "", "");
        LEASI_VERIFYA(Entry->TfcRefIndex >= 0 && Entry->TfcRefIndex < int32_t(TfcRefTable.size()),
            "entry's tfc reference index ({}) is out of bounds ({})", Entry->TfcRefIndex, TfcRefTable.size());

        return (FGuid)TfcRefTable[Entry->TfcRefIndex].TfcGuid;
    }

    FString ManifestLoader::GetTfcName(CTextureEntry const* const Entry) const
    {
        LEASI_CHECKA(Entry != nullptr, "", "");
        LEASI_VERIFYA(Entry->TfcRefIndex >= 0 && Entry->TfcRefIndex < int32_t(TfcRefTable.size()),
            "entry's tfc reference index ({}) is out of bounds ({})", Entry->TfcRefIndex, TfcRefTable.size());

        auto const& TfcName = TfcRefTable[Entry->TfcRefIndex].TfcName;
        LEASI_VERIFYA(FindChar(TfcName, L'\0') != nullptr, "invalid tfc name", "");
        return FString::Printf(L"%s", TfcName);
    }

    bool ManifestLoader::Compare(ManifestLoaderPointer const& Left, ManifestLoaderPointer const& Right)
    {
        return Left->MountPriority >= Right->MountPriority;
    }
}
