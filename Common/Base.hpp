#pragma once

#include <cstdint>
#include <string_view>

#include "spdlog/spdlog.h"
#include <LESDK/Init.hpp>


// ! Common macros.
// ========================================

#define LEASI_UNREACHABLE                   __assume(false)
#define LEASI_BREAK()                       __debugbreak()
#define LEASI_BREAK_SAFE()                  do { if (::IsDebuggerPresent()) { LEASI_BREAK(); } } while (false)

#define LEASI_UNUSED(a)                     (void)a
#define LEASI_UNUSED_2(a, b)                do { (void)a; (void)b; } while (false)
#define LEASI_UNUSED_3(a, b, c)             do { (void)a; (void)b; (void)c; } while (false)
#define LEASI_UNUSED_4(a, b, c, d)          do { (void)a; (void)b; (void)c; (void)d; } while (false)
#define LEASI_UNUSED_5(a, b, c, d, e)       do { (void)a; (void)b; (void)c; (void)d; (void)e; } while (false)
#define LEASI_UNUSED_6(a, b, c, d, e, f)    do { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; } while (false)

#define LEASI_FUNCA                         __FUNCSIG__
#define LEASI_FILEA                         __FILE__
#define LEASI_LINEA                         __LINE__

#define LEASI_FUNCW                         L"" __FUNCSIG__
#define LEASI_FILEW                         L"" __FILE__
#define LEASI_LINEW                         __LINE__

#define _LEASI_VARCAT(a, b)                 a##b
#define LEASI_VARCAT(a, b)                  _LEASI_VARCAT(a, b)


// ! Logger / assertion interface.
// ========================================

#define LEASI_TRACE(...)                    SPDLOG_TRACE(__VA_ARGS__)
#define LEASI_DEBUG(...)                    SPDLOG_DEBUG(__VA_ARGS__)
#define LEASI_INFO(...)                     SPDLOG_INFO(__VA_ARGS__)
#define LEASI_WARN(...)                     SPDLOG_WARN(__VA_ARGS__)
#define LEASI_ERROR(...)                    SPDLOG_ERROR(__VA_ARGS__)
#define LEASI_CRIT(...)                     SPDLOG_CRITICAL(__VA_ARGS__)

namespace Common
{
    void InitializeLoggerDefault();
    /**
     * @brief   Initializes the Globals that are required for basic SDK functionality
     */
    void InitializeRequiredGlobals(::LESDK::Initializer& Init);
}

namespace Details
{
    void OnFatalError(char const* Func, char const* File, int Line, char const* Expr);
}

#define _LEASI_FATALA_IMPL(Expression, Format, ...)                                     \
    do {                                                                                \
        if (Format && strlen(Format) > 0) { LEASI_CRIT(Format, __VA_ARGS__); };         \
        LEASI_BREAK_SAFE();                                                             \
        ::Details::OnFatalError(LEASI_FUNCA, LEASI_FILEA, LEASI_LINEA, Expression);     \
        LEASI_UNREACHABLE;                                                              \
    } while (false)

#define _LEASI_FATALW_IMPL(Expression, Format, ...)                                     \
    do {                                                                                \
        if (Format && wcslen(Format) > 0) { LEASI_CRIT(Format, __VA_ARGS__); };         \
        LEASI_BREAK_SAFE();                                                             \
        ::Details::OnFatalError(LEASI_FUNCA, LEASI_FILEA, LEASI_LINEA, Expression);     \
        LEASI_UNREACHABLE;                                                              \
    } while (false)


#define LEASI_FATALA(Format, ...)                   _LEASI_FATALA_IMPL(nullptr, Format, __VA_ARGS__)
#define LEASI_FATALW(Format, ...)                   _LEASI_FATALW_IMPL(nullptr, Format, __VA_ARGS__)

#define LEASI_VERIFYA(Expression, Format, ...)      do { if (!(Expression)) { _LEASI_FATALA_IMPL(#Expression, Format, __VA_ARGS__); } } while (false)
#define LEASI_VERIFYW(Expression, Format, ...)      do { if (!(Expression)) { _LEASI_FATALW_IMPL(#Expression, Format, __VA_ARGS__); } } while (false)

#ifdef _DEBUG
    #define LEASI_CHECKA(Expression, Format, ...)   LEASI_VERIFYA(Expression, Format, __VA_ARGS__)
    #define LEASI_CHECKW(Expression, Format, ...)   LEASI_VERIFYW(Expression, Format, __VA_ARGS__)
#else
    #define LEASI_CHECKA(Expression, Format, ...)   do { } while (false)
    #define LEASI_CHECKW(Expression, Format, ...)   do { } while (false)
#endif

#define CHECK_RESOLVED(variable)                                                    \
    do {                                                                            \
        LEASI_VERIFYA(variable != nullptr, "failed to resolve " #variable, "");     \
        LEASI_TRACE("resolved " #variable " => {}", (void*)variable);               \
    } while (false)

// ! Miscellaneous types.
// ========================================

class NonCopyable
{
public:
    NonCopyable() = default;
    NonCopyable(NonCopyable const&) = delete;
    NonCopyable& operator=(NonCopyable const&) = delete;
};
