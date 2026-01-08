#include "Common/Base.hpp"
#include <LESDK/Headers.hpp>

void Common::InitializeLoggerDefault()
{
    spdlog::default_logger()->set_pattern("%^[%H:%M:%S.%e] [%l] %v%$");
    spdlog::default_logger()->set_level(spdlog::level::trace);
}

void Common::InitializeRequiredGlobals(::LESDK::Initializer& Init)
{
    GMalloc = Init.ResolveTyped<FMallocLike*>(BUILTIN_GMALLOC_RIP);
    CHECK_RESOLVED(GMalloc);
    
    UObject::GObjObjects = Init.ResolveTyped<TArray<UObject*>>(BUILTIN_GOBOBJECTS_RIP);
    CHECK_RESOLVED(UObject::GObjObjects);
    
    SFXName::GBioNamePools = Init.ResolveTyped<SFXNameEntry const*>(BUILTIN_SFXNAMEPOOLS_RIP);
    CHECK_RESOLVED(SFXName::GBioNamePools);
    
    SFXName::GInitMethod = Init.ResolveTyped<SFXName::tInitMethod>(BUILTIN_SFXNAMEINIT_PHOOK);
    CHECK_RESOLVED(SFXName::GInitMethod);
}

void Details::OnFatalError(char const* const Func, char const* const File, int const Line, char const* const Expr)
{
    char const* const ExprSafe = (Expr != nullptr && strlen(Expr) > 0) ? Expr : "???";
    spdlog::source_loc const Location{ File, Line, Func };
    spdlog::log(Location, spdlog::level::critical, "Fatal assertion: {}", ExprSafe);
    spdlog::log(Location, spdlog::level::critical, "  at: {}:{}", File, Line);
    spdlog::log(Location, spdlog::level::critical, "  func: {}", Func);
    std::abort();
}
