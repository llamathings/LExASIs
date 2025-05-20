#pragma once

#include <LESDK/Headers.hpp>
#include "Common/Base.hpp"


namespace ConvoSniffer
{

    /// Copied from SFXGame_classes.hpp
    enum EConvGUIStyles
    {
        GUI_STYLE_NONE = 0,
        GUI_STYLE_CHARM = 1,
        GUI_STYLE_INTIMIDATE = 2,
        GUI_STYLE_PLAYER_ALERT = 3,
        GUI_STYLE_ILLEGAL = 4,
        GUI_STYLE_MAX = 5
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

    extern bool gb_renderProfile;

    /// Renders profiling HUD for the given @c Conversation onto the given @c Canvas instance.
    void ProfileConversation(UCanvas* Canvas, UBioConversation* Conversation);

}
