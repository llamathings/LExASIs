#pragma once

#include <LESDK/Headers.hpp>
#include <LESDK/Init.hpp>
#include "Common/Base.hpp"


namespace TextureOverride
{

    // ! UTexture2D::Serialize
    // ========================================

#if defined(SDK_TARGET_LE1)
    #define UTEXTURE2D_SERIALIZE_RVA ::LESDK::Address::FromOffset(0x2742b0)
    #define OODLE_DECOMPRESS_RVA     ::LESDK::Address::FromOffset(0x15adb0) // We don't use direct export for ease of setup
#elif defined(SDK_TARGET_LE2)
    #define UTEXTURE2D_SERIALIZE_RVA ::LESDK::Address::FromOffset(0x39ec80)
    #define OODLE_DECOMPRESS_RVA     ::LESDK::Address::FromOffset(0x103ac0) // We don't use direct export for ease of setup
#elif defined(SDK_TARGET_LE3)
    #define UTEXTURE2D_SERIALIZE_RVA ::LESDK::Address::FromOffset(0x3C1FB0)
    #define OODLE_DECOMPRESS_RVA     ::LESDK::Address::FromOffset(0x11fd10) // We don't use direct export for ease of setup
#endif

    using t_UTexture2D_Serialize = void(UTexture2D* Context, void* Archive);
    extern t_UTexture2D_Serialize* UTexture2D_Serialize_orig;
    void UTexture2D_Serialize_hook(UTexture2D* Context, void* Archive);

    using t_OodleDecompress = void* (unsigned int decompressionFlags, void* outPtr, int uncompressedSize, void* inPtr, int compressedSize);
    extern t_OodleDecompress* OodleDecompress;
}
