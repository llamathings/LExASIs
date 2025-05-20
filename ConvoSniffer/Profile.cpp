#include "ConvoSniffer/Profile.hpp"

namespace ConvoSniffer
{
    bool gb_renderProfile = false;

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

        Canvas->DrawTextScaled(FString::Printf(L"Need to display replies: %d",
            (int)Conversation->NeedToDisplayReplies()), 1.f, 1.f);
        Canvas->CurX = s_profilePadding;
        Canvas->CurY += s_profilePaddingSmall;
        Canvas->CurY += s_profilePaddingSmall;

        Canvas->DrawTextScaled(L"Replies:", 1.f, 1.f);
        Canvas->CurX = s_profilePadding;
        Canvas->CurY += s_profilePaddingSmall;
        Canvas->CurY += s_profilePaddingSmall;

        int const   nCurrentEntry = Conversation->m_nCurrentEntry;
        int         nReplyCount = 0;

        if (nCurrentEntry >= 0 && nCurrentEntry < static_cast<int>(Conversation->m_EntryList.Count()))
        {
            FBioDialogEntryNode const& CurrentEntry = Conversation->m_EntryList(nCurrentEntry);
            for (int Index : Conversation->m_lstCurrentReplyIndices)
            {
                if (Index == -1) continue;
                FBioDialogReplyListDetails const& Details = CurrentEntry.ReplyListNew(Index);

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
