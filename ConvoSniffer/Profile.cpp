#include "ConvoSniffer/Profile.hpp"

namespace ConvoSniffer
{

    namespace
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

    wchar_t const* GuiStyleToString(EConvGUIStyles const InValue)
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

    }

    void ProfileConversation(UCanvas* const Canvas, UBioConversation* const Conversation)
    {
        int const Width = Canvas->SizeX;
        //int const Height = Canvas->SizeY;

        static float   s_profilePadding{ 40.f };
        static float   s_profilePaddingSmall{ 10.f };
        static FColor   s_profileColorMain{ 0xff, 0xff, 0xff, 0xff };
        static FColor   s_profileColorEmphasis{ 0x00, 0xff, 0xff, 0xff };

        Canvas->CurX = Canvas->CurY = s_profilePadding;
        Canvas->DrawColor = s_profileColorMain;
        Canvas->DrawTextScaled(FString::Printf(L"ConvoSniffer (LE1) profiler / build = %s", L"" __TIMESTAMP__), 1.f, 1.f);
        Canvas->CurX = s_profilePadding;
        Canvas->CurY += s_profilePaddingSmall;

        Canvas->Draw2DLine(s_profilePadding, Canvas->CurY + s_profilePaddingSmall, Width - s_profilePadding, Canvas->CurY + s_profilePaddingSmall, s_profileColorMain);
        Canvas->CurX = s_profilePadding;
        Canvas->CurY += s_profilePadding;

        Canvas->DrawTextScaled(L"Replies:", 1.f, 1.f);
        Canvas->CurX = s_profilePadding;
        Canvas->CurY += s_profilePaddingSmall;
        Canvas->CurY += s_profilePaddingSmall;

        int const   nCurrentEntry = Conversation->m_nCurrentEntry;
        int         nReplyCount = 0;

        if (nCurrentEntry >= 0 && nCurrentEntry < static_cast<int>(Conversation->m_EntryList.Count()))
        {
            FBioDialogEntryNode const& CurrentEntry = Conversation->m_EntryList(nCurrentEntry);

            int Index = -1;
            for (FBioDialogReplyListDetails const& Details : CurrentEntry.ReplyListNew)
            {
                Index++;

                LEASI_CHECKW(static_cast<UINT>(Details.nIndex) < Conversation->m_ReplyList.Count(), L"reply index out of bounds", L"");
                FBioDialogReplyNode const& Reply = Conversation->m_ReplyList(Details.nIndex);

                if (Reply.bNonTextLine) continue;

                FString const           ReplyParaphrase = Conversation->GetStringInfo(Details.srParaphrase).sText;
                FString const           ReplyText = Conversation->GetStringInfo(Reply.srText).sText;
                EConvGUIStyles const    ReplyStyle = static_cast<EConvGUIStyles>(Reply.eGUIStyle);

                if (!ReplyText.Empty()) nReplyCount++;

                Canvas->CurX = s_profilePadding;
                Canvas->DrawColor = s_profileColorMain;
                Canvas->DrawTextScaled(FString::Printf(L"Reply [%d]:", Index), 1.f, 1.f);

                Canvas->CurX = 100;
                Canvas->DrawColor = s_profileColorEmphasis;
                Canvas->DrawTextScaled(FString::Printf(L"Index = %d / Style = %s / Paraphrase = (%d) %s / Text = (%d) %s",
                    Details.nIndex, GuiStyleToString(ReplyStyle), Details.srParaphrase, *ReplyParaphrase, Reply.srText, *ReplyText), 1.f, 1.f);

                Canvas->CurY += s_profilePaddingSmall;
            }
        }

        if (nReplyCount <= 0)
        {
            Canvas->DrawColor = s_profileColorEmphasis;

            Canvas->DrawTextScaled(L"No valid replies...", 1.f, 1.f);
            Canvas->CurX = s_profilePadding;
            Canvas->CurY += s_profilePaddingSmall;

            Canvas->DrawColor = s_profileColorMain;
        }
    }

}
