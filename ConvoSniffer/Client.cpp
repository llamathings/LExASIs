#include <Windows.h>
#include <wininet.h>

#include <simdutf.h>

#include "Common/Objects.hpp"
#include "ConvoSniffer/Client.hpp"
#include "ConvoSniffer/Profile.hpp"


namespace ConvoSniffer
{
    SnifferClient* gp_snifferClient = nullptr;
    bool gb_renderScaleform = false;


    // ! HttpClient implementation.
    // ========================================

    HttpClient::HttpClient(char const* const InHost, int const InPort)
        : Host{}
        , Port{ InPort }
        , RequestQueue{}
        , RequestIndex{}
        , ProcessThread{}
        , bWantsExit{}
    {
        LEASI_CHECKW(InHost != nullptr, L"", L"");
        Host.AppendAnsi(InHost);

        Win32Internet = InternetOpenW(L"ConvoSniffer-LE1.asi", 0u, NULL, NULL, 0u);
        LEASI_CHECKW(Win32Internet != NULL, L"failed to initialize internet handle: {}", GetLastError());

        Win32Session = InternetConnectW(Win32Internet, *Host, (uint16_t)Port, NULL, NULL, INTERNET_SERVICE_HTTP, 0u, NULL);
        LEASI_CHECKW(Win32Session != NULL, L"failed to initialize http session: {}", GetLastError());

        ProcessThread = std::thread([this]() -> void
            {
                while (!bWantsExit.test())
                {
                    std::unique_lock QueueLock(QueueMutex);

                    if (!RequestQueue.empty())
                    {
                        Request const HttpRequest = RequestQueue.front();
                        RequestQueue.pop_front();
                        QueueLock.unlock();

                        LEASI_INFO(L"request[{}]: method = {}, path = {}", HttpRequest.Index, HttpRequest.Method, *HttpRequest.Path);

                        Response HttpResponse{};
                        if (SendHttp(HttpRequest, &HttpResponse))
                        {
                            if (HttpResponse.Status < 400 || HttpResponse.Status >= 600)
                            {
                                LEASI_INFO(L"response[{}]: status = {}, text = {}", HttpRequest.Index, HttpResponse.Status, *HttpResponse.Body);
                            }
                            else
                            {
                                LEASI_ERROR(L"response[{}]: status = {}, text = {}", HttpRequest.Index, HttpResponse.Status, *HttpResponse.Body);
                            }
                        }
                    }
                    else
                    {
                        RequestCondition.wait(QueueLock);
                    }
                }
            });
    }

    HttpClient::~HttpClient() noexcept
    {
        bWantsExit.test_and_set();
        RequestCondition.notify_all();

        if (InternetCloseHandle(Win32Session) == FALSE)
        {
            // TODO: Log last error code...
        }

        if (InternetCloseHandle(Win32Internet) == FALSE)
        {
            // TODO: Log last error code...
        }
    }

    void HttpClient::QueueRequest(wchar_t const* const InMethod, wchar_t const* const InPath, FString&& InBody)
    {
        LEASI_CHECKW(InMethod != nullptr, L"", L"");
        LEASI_CHECKW(InPath != nullptr, L"", L"");

        if (!bWantsExit.test())
        {
            std::unique_lock QueueLock(QueueMutex);
            RequestQueue.push_back(Request{ RequestIndex++, InMethod, FString(InPath), std::move(InBody) });

            QueueLock.unlock();
            RequestCondition.notify_all();
        }
    }

    bool HttpClient::SendHttp(Request const& InRequest, Response* const OutResponse) const
    {
        bool bSuccess = true;

        HINTERNET const Request = HttpOpenRequestW(
            Win32Session,
            InRequest.Method,
            *InRequest.Path,
            L"HTTP/1.1",
            NULL,
            NULL,
            0u,
            NULL
        );

        if (Request == NULL)
        {
            LEASI_ERROR(L"failed to initialize http request {}", GetLastError());
            return false;
        }

        std::string BodyUtf8{};
        if (!StringToUtf8(InRequest.Body, BodyUtf8))
        {
            LEASI_ERROR(L"failed to convert http request body to utf-8");
            bSuccess = false;
        }

        if (bSuccess && HttpSendRequestW(Request, NULL, 0u, BodyUtf8.data(), (DWORD)BodyUtf8.length()) == FALSE)
        {
            LEASI_ERROR(L"failed to send http request {}", GetLastError());
            bSuccess = false;
        }

        if (bSuccess && OutResponse != nullptr)
        {
            DWORD CodeBufferSize = sizeof(OutResponse->Status);

            if (!HttpQueryInfoW(Request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                &OutResponse->Status, &CodeBufferSize, 0u))
            {
                LEASI_BREAK_SAFE();
                bSuccess = false;
            }

            // TODO: Read to EOF here...

            DWORD TextBufferSize = 1024;
            std::string TextBuffer(TextBufferSize, '\0');

            if (!InternetReadFile(Request, TextBuffer.data(), TextBufferSize, &TextBufferSize))
            {
                LEASI_BREAK_SAFE();
                bSuccess = false;
            }

            TextBuffer.resize(TextBufferSize);
            if (!StringFromUtf8(OutResponse->Body, TextBuffer))
            {
                LEASI_ERROR(L"failed to convert http response body from utf-8");
                bSuccess = false;
            }
        }

        if (InternetCloseHandle(Request) == FALSE)
        {
            LEASI_ERROR(L"failed to close http request: {}", GetLastError());
        }

        return bSuccess;
    }

    bool HttpClient::StringToUtf8(FString const& InString, std::string& OutString)
    {
        auto const Chars = reinterpret_cast<char16_t const*>(InString.Chars());
        auto const Length = static_cast<std::size_t>(InString.Length());

        if (!simdutf::validate_utf16le(Chars, Length))
            return false;

        auto const OutLength = simdutf::utf8_length_from_utf16le(Chars, Length);

        if (OutLength != 0)
        {
            OutString.resize(OutLength);
            simdutf::convert_utf16le_to_utf8(Chars, Length, OutString.data());
        }

        return true;
    }

    bool HttpClient::StringFromUtf8(FString& OutString, std::string const& InString)
    {
        auto const Chars = InString.c_str();
        auto const Length = InString.length();

        if (!simdutf::validate_utf8(Chars, Length))
            return false;

        auto const OutLength = simdutf::utf16_length_from_utf8(Chars, Length);

        if (OutLength != 0)
        {
            OutString.Assign(L'\0', (DWORD)OutLength);
            simdutf::convert_utf8_to_utf16le(Chars, Length, (char16_t*)OutString.Chars());
        }

        return true;
    }


    // ! SnifferClient implementation.
    // ========================================

    SnifferClient::SnifferClient(char const* const InHost, int const InPort)
        : Http{ InHost, InPort }
        , Conversation{ nullptr }
        , nCurrentEntry{ -1 }
    {
        SendKeybinds();
    }

    bool SnifferClient::InConversation() const noexcept
    {
        return Conversation != nullptr;
    }

    UBioConversation* SnifferClient::GetActiveConversation() const noexcept
    {
        return Conversation;
    }

    bool SnifferClient::QueueReplyMapped(int const Index)
    {
        if (Index >= 0 && Index < 16 && Conversation != nullptr)
        {
            for (UINT nReply = 0; nReply < Conversation->m_lstCurrentReplyIndices.Count(); nReply++)
            {
                if (Conversation->m_lstCurrentReplyIndices(nReply) == Index)
                    return Conversation->QueueReply((int)nReply);
            }
        }

        return false;
    }

    void SnifferClient::OnStartConversation(UBioConversation* const InConversation)
    {
        if (Conversation == nullptr)
        {
            // For when the server starts after the game.
            SendKeybinds();

            Conversation = InConversation;
            Http.QueueRequest(L"POST", L"/conversation", FString());
            nCurrentEntry = -1;
            bInitialReplies = false;

            if (!gb_renderScaleform)
            {
                Common::FindFirstObject<UConsole>()
                    ->ConsoleCommand(L"show scaleform");
            }
        }
        else
        {
            LEASI_ERROR("a conversation is already in-progress");
            LEASI_BREAK_SAFE();
        }
    }

    void SnifferClient::OnEndConversation(UBioConversation* const InConversation)
    {
        if (Conversation == InConversation)
        {
            Http.QueueRequest(L"DELETE", L"/conversation", FString());
            Conversation = nullptr;
            nCurrentEntry = -1;

            if (!gb_renderScaleform)
            {
                Common::FindFirstObject<UConsole>()
                    ->ConsoleCommand(L"show scaleform");
            }
        }
        else if (Conversation != nullptr)
        {
            LEASI_ERROR("conversation {:p} is in flight but {:p} is to end", (void*)Conversation, (void*)InConversation);
            LEASI_BREAK_SAFE();
        }
        else
        {
            LEASI_ERROR("no conversations are in-progress");
            LEASI_BREAK_SAFE();
        }
    }

    void SnifferClient::OnQueueReply(UBioConversation* const InConversation, int const Reply)
    {
        LEASI_UNUSED(InConversation);
        LEASI_UNUSED(Reply);

        if (Conversation == InConversation)
        {
            int const Index = (Reply >= 0 && Reply < (int)InConversation->m_lstCurrentReplyIndices.Count())
                ? InConversation->m_lstCurrentReplyIndices(Reply)
                : 15;

            Http.QueueRequest(L"POST", L"/conversation/replies/queue", FString::Printf(L"%d", Index));
        }
        else
        {
            LEASI_ERROR(L"conversation {:p} is in flight but {:p} is to queue reply", (void*)Conversation, (void*)InConversation);
            LEASI_BREAK_SAFE();
        }
    }

    void SnifferClient::OnUpdateConversation(UBioConversation* const InConversation)
    {
        if (Conversation == InConversation)
        {
            if (nCurrentEntry != InConversation->m_nCurrentEntry)
            {
                SendReplyUpdate();
                nCurrentEntry = InConversation->m_nCurrentEntry;
                bInitialReplies = true;
            }
        }
        else
        {
            LEASI_ERROR(L"conversation {:p} is in flight but {:p} is to update", (void*)Conversation, (void*)InConversation);
            LEASI_BREAK_SAFE();
        }
    }

    void SnifferClient::OnVeryFrequentUpdateConversation()
    {
        if (!bInitialReplies && Conversation != nullptr)
        {
            OnUpdateConversation(Conversation);
            bInitialReplies = true;
        }
    }

    void SnifferClient::SendKeybinds()
    {
        // Let's hard-code the keybinds for now...
        Http.QueueRequest(L"PUT", L"/keybinds", L"0;1;2;3;4;5;6;7;8;9;A;B;C;D;E;F");
    }

    void SnifferClient::SendReplyUpdate()
    {
        FString Buffer{};
        BuildReplyUpdate(Conversation, Buffer);
        SendReplyUpdate(std::move(Buffer));
    }

    void SnifferClient::SendReplyUpdate(FString&& InBuffer)
    {
        Http.QueueRequest(L"PUT", L"/conversation/replies", std::move(InBuffer));
    }

    void SnifferClient::BuildReplyUpdate
    (
        UBioConversation* const     InConversation,
        FString&                    OutBuffer
    )
    {
        OutBuffer.Clear();
        OutBuffer.Reserve(1024);

        UEngine* const Engine = Common::FindFirstObject<UEngine>();
        LEASI_CHECKW(Engine != nullptr, L"failed to retrieve engine", L"");
        ABioWorldInfo* const WorldInfo = Engine->GetCurrentWorldInfo()->Cast<ABioWorldInfo>();
        LEASI_CHECKW(WorldInfo != nullptr, L"failed to retrieve world info", L"");

        int const nCurrentEntry = InConversation->m_nCurrentEntry;
        std::vector<ReplyBundle> ReplyBundles{};

        if (nCurrentEntry >= 0 && nCurrentEntry < static_cast<int>(InConversation->m_EntryList.Count()))
        {
            FBioDialogEntryNode const& CurrentEntry = InConversation->m_EntryList(nCurrentEntry);
            for (int Index : InConversation->m_lstCurrentReplyIndices)
            {
                if (Index == -1) continue;
                FBioDialogReplyListDetails const& Details = CurrentEntry.ReplyListNew(Index);

                LEASI_CHECKW(static_cast<UINT>(Details.nIndex) < InConversation->m_ReplyList.Count(), L"reply index out of bounds", L"");
                FBioDialogReplyNode const& Reply = InConversation->m_ReplyList(Details.nIndex);

                //if (!CheckReplyCondition(WorldInfo, InConversation, Reply)) continue;
                ReplyBundle& Bundle = ReplyBundles.emplace_back();

                Bundle.Index = Index;
                Bundle.Style = GuiStyleToString(static_cast<EConvGUIStyles>(Reply.eGUIStyle));
                Bundle.Category = ReplyCategoryToString(static_cast<EReplyCategory>(Details.Category));
                Bundle.Paraphrase = InConversation->GetStringInfo(Details.srParaphrase).sText;
                Bundle.Text = InConversation->GetStringInfo(Reply.srText).sText;

                if (!CheckReplyCondition(WorldInfo, InConversation, Reply))
                    Bundle.Style = GuiStyleToString(GUI_STYLE_ILLEGAL);

                if (Bundle.Text.Empty()) ReplyBundles.pop_back();
                if (Bundle.Text.Contains(L"\n")) LEASI_BREAK_SAFE();
            }
        }

        OutBuffer.AppendFormat(L"%llu\n", ReplyBundles.size());

        for (ReplyBundle const& Bundle : ReplyBundles)
        {
            OutBuffer.AppendFormat(L"\n");
            OutBuffer.AppendFormat(L"%d\n", Bundle.Index);
            OutBuffer.AppendFormat(L"%s\n", Bundle.Style);
            OutBuffer.AppendFormat(L"%s\n", Bundle.Category);
            OutBuffer.AppendFormat(L"%s\n", *Bundle.Paraphrase);
            OutBuffer.AppendFormat(L"%s\n", *Bundle.Text);
        }
    }

    bool SnifferClient::CheckReplyCondition
    (
        ABioWorldInfo* const            WorldInfo,
        UBioConversation* const         InConversation,
        FBioDialogReplyNode const&      InReply
    )
    {
        if (InReply.bNonTextLine)
            return false;

        wchar_t const* const ConditionalType = InReply.bFireConditional ? L"conditional" : L"bool";

        LEASI_TRACE(L"Evaluating reply: {}", *InConversation->GetStringInfo(InReply.srText).sText);
        LEASI_TRACE(L"  id = {}, param = {}, type = {}", InReply.nConditionalFunc, InReply.nConditionalParam, ConditionalType);

        if (InReply.bFireConditional)
        {
            if (InReply.nConditionalFunc >= 0)
            {
                SFXName const ClassName(L"BioAutoConditionals", 0);
                SFXName const FunctionName(*FString::Printf(L"F%d", InReply.nConditionalFunc), 0);

                Common::TypedObjectIterator<UFunction> Iterator{};
                for (; Iterator; ++Iterator)
                {
                    UFunction* const    Function = *Iterator;
                    UClass* const       Class = Function->Outer ? Function->Outer->Cast<UClass>() : nullptr;

                    if (Function->Name == FunctionName && Class != nullptr && Class->Name == ClassName)
                    {
                        LEASI_CHECKW((Function->FunctionFlags & 0x400) == 0,
                            L"native conditional: {}", *Function->GetFullName());

                        UObject* const      Context = Class->ClassDefaultObject;
                        ConditionalParms    Parms{ WorldInfo, InReply.nConditionalParam, 0u };

                        Context->ProcessEvent(Function, &Parms, NULL);

                        if (Parms.ReturnValue == FALSE)
                            return false;

                        break;
                    }
                }
            }
        }
        else
        {
            UBioGlobalVariableTable* const VarTable = WorldInfo->GetGlobalVariables();
            LEASI_CHECKW(VarTable != nullptr, L"failed to retrieve global variables", L"");

            int const Evaluated = static_cast<int>(VarTable->GetBool(InReply.nConditionalFunc));
            int const Expected = InReply.nConditionalParam;

            LEASI_CHECKW(Expected == 0 || Expected == 1,
                L"unexpected bool {} param: {}", InReply.nConditionalFunc, Expected);

            if (Evaluated != Expected)
                return false;
        }

        return true;
    }
}
