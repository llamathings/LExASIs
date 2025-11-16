asi_target (ConvoSniffer
  SHARED
  GAMES "LE1"
  SOURCES
    "Client.cpp"
    "Client.hpp"
    "Config.hpp"
    "Entry.cpp"
    "Entry.hpp"
    "Hooks.cpp"
    "Hooks.hpp"
    "Profile.cpp"
    "Profile.hpp"
  VLINKS
    "Common"
    "LESDK"
  LINKS
    "simdutf"
    Wininet.lib
  )
