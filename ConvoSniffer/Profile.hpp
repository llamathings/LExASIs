#pragma once

#include <LESDK/Headers.hpp>
#include "Common/Base.hpp"


namespace ConvoSniffer
{

    /// Renders profiling HUD for the given @c Conversation onto the given @c Canvas instance.
    void ProfileConversation(UCanvas* Canvas, UBioConversation* Conversation);

}
