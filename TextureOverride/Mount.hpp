#pragma once

#include <filesystem>
#include "LESDK/Headers.hpp"

namespace TextureOverride
{

    /**
     * @brief       Tries to read mount priority for a DLC with the given path.
     * @param[in]   DlcPath - path to the DLC folder.
     * @param[in]   GameIndex - index of the game (1 = LE1, 2 = LE2, etc.)
     * @param[out]  pOutError - mandatory string variable to receive an error message.
     * @return      Mount priority if successful, or a negative number in case of an error.
     */
    int TryReadMountPriority(std::filesystem::path const& DlcPath, int GameIndex, FString* pOutError);

#pragma pack(push, 1)

    struct LE2MountPartial
    {
        uint32_t        VerPackage;     // 684
        uint32_t        VerLicensee;    // 168
        uint32_t        VerCook;        // 65643
        uint32_t        ModuleId;       // 300
        uint32_t        InfoVersion;    // 4
        FGuid           TfcGuid;        // ...
        uint32_t        Unknown0;       // 0
        uint32_t        Flags;          // 2

        // Followed by variable-sized fields:
        //   FString DlcName;
        //   int32_t DlcNameStrRef;
        //   FString FolderName;
        //   ...
    };

    struct LE3MountPartial
    {
        uint32_t        Unknown0;       // 1
        uint32_t        VerPackage;     // 685
        uint32_t        VerLicensee;    // 205
        uint32_t        VerCook;        // 196715
        uint32_t        ModuleId;       // 3200
        uint32_t        Unknown1;       // 0
        uint32_t        Flags;          // 9
        uint32_t        TlkId0;         // 727500
        uint32_t        TlkId1;         // 727500

        // Followed by a bunch of zeroes:
        //   ...
    };

#pragma pack(pop)

}