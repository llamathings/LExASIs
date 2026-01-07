#pragma once

#include <LESDK/Headers.hpp>
#include <LESDK/Init.hpp>
#include <SPI.h>


namespace KismetLogger
{
    void InitializeGlobals(::LESDK::Initializer& Init);
    void InitializeHooks(::LESDK::Initializer& Init);
}
