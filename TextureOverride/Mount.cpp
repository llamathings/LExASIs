#include "TextureOverride/Mount.hpp"
#include "Common/Base.hpp"
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace
{

    class ScopedFile final : public NonCopyable
    {
        FILE*       FileHandle{};
        fs::path    FilePath{};
        FString*    pOutError{};

    public:

        ScopedFile(fs::path const& InPath, FString* const InOutError)
            : FilePath{ InPath }
            , pOutError{ InOutError }
        {
            FileHandle = _wfopen(FilePath.c_str(), L"rb");
            if (FileHandle == nullptr)
            {
                *pOutError = FString::Printf(L"failed to open mount.dlc: %s", _wcserror(errno));
            }
        }

        ~ScopedFile()
        {
            if (FileHandle != nullptr
                && fclose(FileHandle) != 0)
            {
                *pOutError = FString::Printf(L"failed to close %s", FilePath.c_str());
            }
        }

        operator bool() const { return FileHandle != nullptr; }
        FILE* operator*() const { return FileHandle; }

        std::stringstream ReadAllText() const
        {
            if (!this->operator bool())
                return std::stringstream{};

            // TODO: Add error handling here.

            long const Origin = ftell(this->operator*());
            fseek(this->operator*(), 0, SEEK_END);

            long const Length = ftell(this->operator*());
            fseek(this->operator*(), Origin, SEEK_SET);

            std::string Buffer(size_t(Length), '\0');

            fread(Buffer.data(), sizeof(decltype(Buffer)::value_type), size_t(Length), this->operator*());
            fseek(this->operator*(), Origin, SEEK_SET);

            return std::stringstream(std::move(Buffer));
        }
    };

}

namespace TextureOverride
{
    int TryReadMountPriority(
        std::filesystem::path const& DlcPath,
        int const GameIndex, FString* const pOutError)
    {
        // These are bad programming errors, always crash if detected.
        LEASI_VERIFYW(GameIndex > 0 && GameIndex <= 3, L"invalid game index: {}", GameIndex);
        LEASI_VERIFYW(pOutError != nullptr, L"null error variable", L"");

        if (GameIndex == 1)
        {
            // LE1 uses our own autoload mechanism, mount priority is in the .ini file.

            ScopedFile File{ DlcPath / "AutoLoad.ini", pOutError };
            std::stringstream Contents{ File.ReadAllText() };

            if (File)
            {
                std::string Line{};
                while (std::getline(Contents, Line))
                {
                    int MountPriority = -1;
                    if (std::sscanf(Line.c_str(), " ModMount = %d", &MountPriority) == 1)
                    {
                        if (MountPriority >= 0)
                        {
                            pOutError->Clear();
                            return MountPriority;
                        }
                        else
                        {
                            *pOutError = FString::Printf(L"found negative ModMount value: %d", MountPriority);
                        }
                    }
                }

                *pOutError = FString::Printf(L"failed to find ModMount .ini line");
            }
        }
        else if (GameIndex == 2)
        {
            // LE2 uses binary mount file.

            LE2MountPartial MountPartial{};
            ScopedFile File{ DlcPath / "CookedPCConsole" / "Mount.dlc", pOutError };

            if (File)
            {
                static constexpr size_t k_readSize = sizeof(MountPartial);
                if (fread(&MountPartial, 1, k_readSize, *File) != k_readSize)
                {
                    *pOutError = FString::Printf(L"failed to read %d byte(s) of mount file", int(k_readSize));
                    return -1;
                }

                pOutError->Clear();
                return static_cast<int>(MountPartial.ModuleId);
            }
        }
        else if (GameIndex == 3)
        {
            // LE3 and MLN use a different binary mount file.

            LE3MountPartial MountPartial{};
            ScopedFile File{ DlcPath / "CookedPCConsole" / "Mount.dlc", pOutError };

            if (File)
            {
                static constexpr size_t k_readSize = sizeof(MountPartial);
                if (fread(&MountPartial, 1, k_readSize, *File) != k_readSize)
                {
                    *pOutError = FString::Printf(L"failed to read %d byte(s) of mount file", int(k_readSize));
                    return -1;
                }

                pOutError->Clear();
                return static_cast<int>(MountPartial.ModuleId);
            }
        }
        else
        {
            *pOutError = FString::Printf(L"failed to match game for index %d", GameIndex);
        }
        
        return -1;
    }
}
