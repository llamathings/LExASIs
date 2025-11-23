#pragma once

#include <LESDK/Headers.hpp>
#include <LESDK/Init.hpp>
#include "Common/Base.hpp"


namespace TextureOverride
{

#if defined(SDK_TARGET_LE1)
    #define UGAMEENGINE_EXEC_RVA        ::LESDK::Address::FromOffset(0x3BD5D0)
    #define UTEXTURE2D_SERIALIZE_RVA    ::LESDK::Address::FromOffset(0x2742b0)
    #define OODLE_DECOMPRESS_RVA        ::LESDK::Address::FromOffset(0x15adb0)
#elif defined(SDK_TARGET_LE2)
    #define UGAMEENGINE_EXEC_RVA        ::LESDK::Address::FromOffset(0x5383C0)
    #define UTEXTURE2D_SERIALIZE_RVA    ::LESDK::Address::FromOffset(0x39ec80)
    #define OODLE_DECOMPRESS_RVA        ::LESDK::Address::FromOffset(0x103ac0)
#elif defined(SDK_TARGET_LE3)
    #define UGAMEENGINE_EXEC_RVA        ::LESDK::Address::FromOffset(0x541920)
    #define UTEXTURE2D_SERIALIZE_RVA    ::LESDK::Address::FromOffset(0x3C1FB0)
    #define OODLE_DECOMPRESS_RVA        ::LESDK::Address::FromOffset(0x11fd10)
#endif

    // ! UGameEngine::Exec
    // ========================================

    using t_UGameEngine_Exec = DWORD(UGameEngine* Context, WCHAR const* Command, void* Archive);
    extern t_UGameEngine_Exec* UGameEngine_Exec_orig;
    DWORD UGameEngine_Exec_hook(UGameEngine* Context, WCHAR const* Command, void* Archive);

    // ! UTexture2D::Serialize
    // ========================================

    using t_UTexture2D_Serialize = void(UTexture2D* Context, void* Archive);
    extern t_UTexture2D_Serialize* UTexture2D_Serialize_orig;
    void UTexture2D_Serialize_hook(UTexture2D* Context, void* Archive);

    // ! OodleDecompress
    // ========================================

    using t_OodleDecompress = void* (unsigned int decompressionFlags, void* outPtr, int uncompressedSize, void* inPtr, int compressedSize);
    extern t_OodleDecompress* OodleDecompress;
}
