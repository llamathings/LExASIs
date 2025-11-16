#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

#include <LESDK/Headers.hpp>
#include "Common/Base.hpp"

namespace ConvoSniffer
{
    class SnifferClient;

    extern SnifferClient*       gp_snifferClient;
    extern bool                 gb_renderScaleform;

    class HttpClient final : public NonCopyable
    {
    public:

        HttpClient(char const* InHost, int InPort);
        ~HttpClient() noexcept;

        void QueueRequest(wchar_t const* InMethod, wchar_t const* InPath, FString&& InBody);

    private:

        struct Request final
        {
            int             Index{};
            wchar_t const*  Method{};
            FString         Path{};
            FString         Body{};
        };

        struct Response final
        {
            DWORD           Status{};
            FString         Body{};
        };

        bool SendHttp(Request const& InRequest, Response* OutResponse = nullptr) const;
        static bool StringToUtf8(FString const& InString, std::string& OutString);
        static bool StringFromUtf8(FString& OutString, std::string const& InString);

        LPVOID                      Win32Internet;
        LPVOID                      Win32Session;

        FString                     Host;
        int                         Port;

        std::mutex                  QueueMutex;
        std::condition_variable     RequestCondition;
        std::deque<Request>         RequestQueue;
        int                         RequestIndex;
        std::thread                 ProcessThread;
        std::atomic_flag            bWantsExit;
    };

    class SnifferClient final : public NonCopyable
    {
    public:

        SnifferClient(char const* InHost, int InPort);
        ~SnifferClient() noexcept = default;

        bool InConversation() const noexcept;
        UBioConversation* GetActiveConversation() const noexcept;

        bool QueueReplyMapped(int Index);

        void OnStartConversation(UBioConversation* InConversation);
        void OnEndConversation(UBioConversation* InConversation);
        void OnQueueReply(UBioConversation* InConversation, int Reply);
        void OnUpdateConversation(UBioConversation* InConversation);
        void OnVeryFrequentUpdateConversation();

    private:

        void SendKeybinds();
        void SendReplyUpdate();
        void SendReplyUpdate(FString&& InBuffer);

        struct ConditionalParms final
        {
            ABioWorldInfo*      BioWorld;
            int                 Argument;
            unsigned long       ReturnValue;
        };

        static void BuildReplyUpdate(UBioConversation* InConversation, FString& OutBuffer);
        static bool CheckReplyCondition(ABioWorldInfo* WorldInfo, UBioConversation* InConversation, FBioDialogReplyNode const& InReply);

        struct ReplyBundle final
        {
            int                 Index;
            wchar_t const*      Style;
            wchar_t const*      Category;
            FString             Paraphrase;
            FString             Text;
        };

        HttpClient              Http;
        UBioConversation*       Conversation;
        int                     nCurrentEntry;
        bool                    bInitialReplies;
    };
}
