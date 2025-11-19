#pragma once

#include <LESDK/Headers.hpp>
#include <LESDK/Init.hpp>
#include "Common/Base.hpp"


#define CONVO_SNIFFER_EXEC_PHOOK ::LESDK::Address::FromPostHook(/* 40 55 56 57 41 */ "54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? 48 81 EC 60 0A 00 00 48 C7 45 ?? ?? ?? ?? ?? 48 89 9C 24 ?? ?? ?? ??")
#define CONVO_SNIFFER_INPUTKEY_PAT ::LESDK::Address::FromPattern("48 89 5C 24 ?? 55 57 41 54 41 55 41 57 48 81 EC 80 00 00 00")
#define CONVO_SNIFFER_STARTCONVERSATION_PAT ::LESDK::Address::FromPattern("40 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 70 48 C7 45 ?? FE FF FF FF 48 89 9C 24 ?? ?? ?? ?? 0F 29 74 24 ?? 49 8B F8")
#define CONVO_SNIFFER_ENDCONVERSATION_PAT ::LESDK::Address::FromPattern("40 56 48 83 EC 40 83 89 ?? ?? ?? ?? 04")
#define CONVO_SNIFFER_SELECTREPLY_PAT ::LESDK::Address::FromPattern("48 8B C4 56 57 41 54 41 56 41 57 48 83 EC 60 48 C7 40 ?? FE FF FF FF 48 89 58 ?? 48 89 68 ?? 48 63 DA")
#define CONVO_SNIFFER_QUEUEREPLY_PAT ::LESDK::Address::FromPattern("48 83 EC 28 C7 81 ?? ?? ?? ?? FF FF FF FF")


namespace ConvoSniffer
{
    // ! UObject hooks.
    // ========================================

    using t_UObject_ProcessEvent = void(UObject* Context, UFunction* Function, void* Parms, void* Result);
    extern t_UObject_ProcessEvent* UObject_ProcessEvent_orig;
    void UObject_ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result);

    using t_UObject_ProcessInternal = void(UObject* Context, FFrame* Stack, void* Result);
    extern t_UObject_ProcessInternal* UObject_ProcessInternal_orig;
    void UObject_ProcessInternal_hook(UObject* Context, FFrame* Stack, void* Result);

    using t_UObject_CallFunction = void(UObject* Context, FFrame* Stack, void* Result, UFunction* Function);
    extern t_UObject_CallFunction* UObject_CallFunction_orig;
    void UObject_CallFunction_hook(UObject* Context, FFrame* Stack, void* Result, UFunction* Function);


    // ! UGameEngine hooks.
    // ========================================

    using t_UGameEngine_Exec = DWORD(UGameEngine* Context, WCHAR const* Command, void* Archive);
    extern t_UGameEngine_Exec* UGameEngine_Exec_orig;
    DWORD UGameEngine_Exec_hook(UGameEngine* Context, WCHAR const* Command, void* Archive);


    // ! UGameViewportClient hooks.
    // ========================================

    using t_UGameViewportClient_InputKey = DWORD(UGameViewportClient* Context, void* Viewport, DWORD Unknown0, SFXName Key, int EventType, DWORD Unknown1, DWORD Unknown2);
    extern t_UGameViewportClient_InputKey* UGameViewportClient_InputKey_orig;
    DWORD UGameViewportClient_InputKey_hook(UGameViewportClient* Context, void* Viewport, DWORD Unknown0, SFXName Key, int EventType, DWORD Unknown1, DWORD Unknown2);


    // ! UBioConversation hooks.
    // ========================================

    using t_UBioConversation_StartConversation = UBOOL(UBioConversation* Context, AActor* Owner, AActor* Player);
    extern t_UBioConversation_StartConversation* UBioConversation_StartConversation_orig;
    UBOOL UBioConversation_StartConversation_hook(UBioConversation* Context, AActor* Owner, AActor* Player);

    using t_UBioConversation_EndConversation = void(UBioConversation* Context);
    extern t_UBioConversation_EndConversation* UBioConversation_EndConversation_orig;
    void UBioConversation_EndConversation_hook(UBioConversation* Context);

    using t_UBioConversation_SelectReply = UBOOL(UBioConversation* Context, int Reply);
    extern t_UBioConversation_SelectReply* UBioConversation_SelectReply_orig;
    UBOOL UBioConversation_SelectReply_hook(UBioConversation* Context, int Reply);

    using t_UBioConversation_QueueReply = UBOOL(UBioConversation* Context, int Reply);
    extern t_UBioConversation_QueueReply* UBioConversation_QueueReply_orig;
    UBOOL UBioConversation_QueueReply_hook(UBioConversation* Context, int Reply);

}
