#pragma once

#include <LESDK/Headers.hpp>
#include "Common/Base.hpp"


namespace ConvoSniffer
{

    /// Copied from SFXGame_classes.hpp
    enum EConvGUIStyles : BYTE
    {
        GUI_STYLE_NONE = 0,
        GUI_STYLE_CHARM = 1,
        GUI_STYLE_INTIMIDATE = 2,
        GUI_STYLE_PLAYER_ALERT = 3,
        GUI_STYLE_ILLEGAL = 4,
        GUI_STYLE_MAX = 5
    };

    /// Copied from SFXGame_classes.hpp
    enum EReplyCategory : BYTE
    {
        REPLY_CATEGORY_DEFAULT = 0,
        REPLY_CATEGORY_AGREE = 1,
        REPLY_CATEGORY_DISAGREE = 2,
        REPLY_CATEGORY_FRIENDLY = 3,
        REPLY_CATEGORY_HOSTILE = 4,
        REPLY_CATEGORY_INVESTIGATE = 5,
        REPLY_CATEGORY_MAX = 6
    };

    inline wchar_t const* GuiStyleToString(EConvGUIStyles const InValue)
    {
        switch (InValue)
        {
        case GUI_STYLE_NONE: return L"NONE";
        case GUI_STYLE_CHARM: return L"CHARM";
        case GUI_STYLE_INTIMIDATE: return L"INTIMIDATE";
        case GUI_STYLE_PLAYER_ALERT: return L"PLAYER_ALERT";
        case GUI_STYLE_ILLEGAL: return L"ILLEGAL";
        default: return L"<N/A>";
        }
    }

    inline wchar_t const* ReplyCategoryToString(EReplyCategory const InValue)
    {
        switch (InValue)
        {
        case REPLY_CATEGORY_DEFAULT: return L"DEFAULT";
        case REPLY_CATEGORY_AGREE: return L"AGREE";
        case REPLY_CATEGORY_DISAGREE: return L"DISAGREE";
        case REPLY_CATEGORY_FRIENDLY: return L"FRIENDLY";
        case REPLY_CATEGORY_HOSTILE: return L"HOSTILE";
        case REPLY_CATEGORY_INVESTIGATE: return L"INVESTIGATE";
        default: return L"<N/A>";
        }
    }

    extern bool gb_renderProfile;

    /// Renders profiling HUD for the given @c Conversation onto the given @c Canvas instance.
    void ProfileConversation(UCanvas* Canvas, UBioConversation* Conversation);

}
