cmake_minimum_required (VERSION 3.27)
project (LExASIs VERSION 0.1.0 LANGUAGES CXX)


# ! Global configuration.
# ========================================

cmake_policy (SET CMP0057 NEW)
cmake_policy (SET CMP0077 NEW)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set_property (GLOBAL PROPERTY USER_FOLDERS ON)

if (DEFINED ENV{LEASI_CMAKE_PRESET})
  set (LEASI_CMAKE_PRESET $ENV{LEASI_CMAKE_PRESET})
  message ("-- Environment selected preset: ${LEASI_CMAKE_PRESET}")
else ()
  message (FATAL_ERROR "!! Environment does not select preset via LEASI_CMAKE_PRESET")
endif ()


# ! Build configuration.
# ========================================

option (LEASI_BUILD_LE1 "Build LE1 targets?" ON)
option (LEASI_BUILD_LE2 "Build LE2 targets?" ON)
option (LEASI_BUILD_LE3 "Build LE3 targets?" ON)

if (CMAKE_GENERATOR MATCHES "Visual Studio 17 2022")
  set (LEASI_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Build/$<CONFIG>)
  message ("-- Detected supported generator: ${CMAKE_GENERATOR}")
  message ("-- Configured output path: ${LEASI_OUTPUT_DIRECTORY}")
else ()
  message (FATAL_ERROR "!! Detected unsupported generator: ${CMAKE_GENERATOR}")
endif ()

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LEASI_OUTPUT_DIRECTORY})
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LEASI_OUTPUT_DIRECTORY})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${LEASI_OUTPUT_DIRECTORY})

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND WIN32)

  add_compile_definitions ( "_CRT_SECURE_NO_WARNINGS" "NOMINMAX" )

  add_compile_options ( "/MP" "/utf-8" "/Zi"
    "$<$<IN_LIST:$<CONFIG>,DEBUG>:/MDd;/Od;/JMC>"
    "$<$<IN_LIST:$<CONFIG>,RELEASE>:/MD;/O2>"
  )

  add_link_options ( "/DEBUG"
    "$<$<IN_LIST:$<CONFIG>,DEBUG>:/OPT:NOREF;/OPT:NOICF>"
    "$<$<IN_LIST:$<CONFIG>,RELEASE>:/OPT:REF;/OPT:ICF;/INCREMENTAL:NO>"
  )

  set (LEASI_COMPILE_DEFINITIONS
    "SPDLOG_ACTIVE_LEVEL=0"  # Log TRACE and up.
    "$<$<IN_LIST:$<CONFIG>,DEBUG>:_DEBUG>"
  )

  set (LEASI_COMPILE_OPTIONS
    "/W4" "/WX" "/permissive"
    "/wd4121"  # -warn: alignment of member was sensitive to padding
    "/wd4127"  # -warn: conditional expression is constant
    "/wd4702"  # -warn: fuck msvc
  )

  set (LEASI_LINK_OPTIONS
    # ...
  )

else ()

  # Note that compiler conditions are explicitly paired with host system checks.
  message (FATAL_ERROR "!! Unsupported compiler ${CMAKE_CXX_COMPILER_ID} on ${CMAKE_SYSTEM_NAME}")

endif ()


# ! Target functions.
# ========================================

# Internal function that implements ASI_TARGET.
function (_ASI_TARGET_IMPL PROJ_TARGET PROJ_TYPE)
  cmake_parse_arguments (PROJ "LE1;LE2;LE3" "" "SOURCES;LINKS;VLINKS" ${ARGN})

  set (LIBRARY_LINKS)

  foreach (PROJ_LINK IN LISTS PROJ_LINKS)
    list (APPEND LIBRARY_LINKS "${PROJ_LINK}")
  endforeach ()

  if (PROJ_LE1)
    set (PROJ_SUFFIX "LE1")
  elseif (PROJ_LE2)
    set (PROJ_SUFFIX "LE2")
  elseif (PROJ_LE3)
    set (PROJ_SUFFIX "LE3")
  endif ()

  foreach (PROJ_VLINK IN LISTS PROJ_VLINKS)
    list (APPEND LIBRARY_LINKS "${PROJ_VLINK}-${PROJ_SUFFIX}")
  endforeach ()


  set (ORIG_TARGET "${PROJ_TARGET}")
  set (PROJ_TARGET "${PROJ_TARGET}-${PROJ_SUFFIX}")

  if (PROJ_TYPE STREQUAL "STATIC")
    add_library (${PROJ_TARGET} STATIC ${PROJ_SOURCES})
  elseif (PROJ_TYPE STREQUAL "SHARED")
    add_library (${PROJ_TARGET} SHARED ${PROJ_SOURCES})
  endif ()

  target_compile_definitions (${PROJ_TARGET} PUBLIC ${LEASI_COMPILE_DEFINITIONS})
  target_compile_options (${PROJ_TARGET} PUBLIC ${LEASI_COMPILE_OPTIONS})
  target_include_directories (${PROJ_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
  target_link_libraries (${PROJ_TARGET} PUBLIC ${LIBRARY_LINKS})
  target_link_options (${PROJ_TARGET} PUBLIC ${LEASI_LINK_OPTIONS})

  source_group (TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${PROJ_SOURCES})

  if (PROJ_TYPE STREQUAL "SHARED")
    set_target_properties (${PROJ_TARGET} PROPERTIES
      OUTPUT_NAME "${PROJ_SUFFIX}${ORIG_TARGET}" SUFFIX ".asi")
  endif ()

endfunction ()

# Sets up an ASI library target according to our conventions.
function (ASI_TARGET PROJ_TARGET PROJ_TYPE)
  cmake_parse_arguments (PROJ "" "" "GAMES;SOURCES;LINKS;VLINKS" ${ARGN})

  list (TRANSFORM PROJ_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")

  set (VALID_TYPES "STATIC" "SHARED")
  if (NOT ${PROJ_TYPE} IN_LIST VALID_TYPES)
    message (FATAL_ERROR "!! Invalid ASI target type: ${PROJ_TARGET}")
  endif ()

  set (VALID_GAMES "LE1" "LE2" "LE3")
  foreach (PROJ_GAME IN LISTS PROJ_GAMES)
    if (NOT ${PROJ_GAME} IN_LIST VALID_GAMES)
      message (FATAL_ERROR "!! Invalid target game: ${PROJ_GAME}")
    endif ()

    if (PROJ_GAME STREQUAL "LE1" AND LEASI_BUILD_LE1)
      _asi_target_impl (${PROJ_TARGET} ${PROJ_TYPE} LE1 SOURCES ${PROJ_SOURCES} LINKS ${PROJ_LINKS} VLINKS ${PROJ_VLINKS})

      if (DEFINED ENV{LEASI_GAME1_WIN64} AND PROJ_TYPE STREQUAL "SHARED")
        set (LEASI_GAME1_WIN64 "$ENV{LEASI_GAME1_WIN64}")
        message ("-- User-provided Mass Effect 1 (LE) Win64 folder: ${LEASI_GAME1_WIN64}")

        set (COPY_SOURCE $<TARGET_FILE:${PROJ_TARGET}-LE1>)
        set (COPY_DEST ${LEASI_GAME1_WIN64}/ASI/$<TARGET_FILE_NAME:${PROJ_TARGET}-LE1>)

        add_custom_command (TARGET ${PROJ_TARGET}-LE1 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${COPY_SOURCE} ${COPY_DEST})

        set_target_properties (${PROJ_TARGET}-LE1 PROPERTIES
          VS_DEBUGGER_COMMAND "${LEASI_GAME1_WIN64}/MassEffect1.exe"
          VS_DEBUGGER_COMMAND_ARGUMENTS "-enableminidumps -nosplash"
          VS_DEBUGGER_WORKING_DIRECTORY "${LEASI_GAME1_WIN64}"
        )
      endif ()
    endif ()

    if (PROJ_GAME STREQUAL "LE2" AND LEASI_BUILD_LE2)
      _asi_target_impl (${PROJ_TARGET} ${PROJ_TYPE} LE2 SOURCES ${PROJ_SOURCES} LINKS ${PROJ_LINKS} VLINKS ${PROJ_VLINKS})

      if (DEFINED ENV{LEASI_GAME2_WIN64} AND PROJ_TYPE STREQUAL "SHARED")
        set (LEASI_GAME2_WIN64 "$ENV{LEASI_GAME2_WIN64}")
        message ("-- User-provided Mass Effect 2 (LE) Win64 folder: ${LEASI_GAME2_WIN64}")

        set (COPY_SOURCE $<TARGET_FILE:${PROJ_TARGET}-LE2>)
        set (COPY_DEST ${LEASI_GAME2_WIN64}/ASI/$<TARGET_FILE_NAME:${PROJ_TARGET}-LE2>)

        add_custom_command (TARGET ${PROJ_TARGET}-LE2 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${COPY_SOURCE} ${COPY_DEST})

        set_target_properties (${PROJ_TARGET}-LE2 PROPERTIES
          VS_DEBUGGER_COMMAND "${LEASI_GAME2_WIN64}/MassEffect2.exe"
          VS_DEBUGGER_COMMAND_ARGUMENTS "-enableminidumps -nosplash"
          VS_DEBUGGER_WORKING_DIRECTORY "${LEASI_GAME2_WIN64}"
        )
      endif ()
    endif ()

    if (PROJ_GAME STREQUAL "LE3" AND LEASI_BUILD_LE3)
      _asi_target_impl (${PROJ_TARGET} ${PROJ_TYPE} LE3 SOURCES ${PROJ_SOURCES} LINKS ${PROJ_LINKS} VLINKS ${PROJ_VLINKS})

      if (DEFINED ENV{LEASI_GAME3_WIN64} AND PROJ_TYPE STREQUAL "SHARED")
        set (LEASI_GAME3_WIN64 "$ENV{LEASI_GAME3_WIN64}")
        message ("-- User-provided Mass Effect 3 (LE) Win64 folder: ${LEASI_GAME3_WIN64}")

        set (COPY_SOURCE $<TARGET_FILE:${PROJ_TARGET}-LE3>)
        set (COPY_DEST ${LEASI_GAME3_WIN64}/ASI/$<TARGET_FILE_NAME:${PROJ_TARGET}-LE3>)

        add_custom_command (TARGET ${PROJ_TARGET}-LE3 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${COPY_SOURCE} ${COPY_DEST})

        set_target_properties (${PROJ_TARGET}-LE3 PROPERTIES
          VS_DEBUGGER_COMMAND "${LEASI_GAME3_WIN64}/MassEffect3.exe"
          VS_DEBUGGER_COMMAND_ARGUMENTS "-enableminidumps -nosplash"
          VS_DEBUGGER_WORKING_DIRECTORY "${LEASI_GAME3_WIN64}"
        )
      endif ()
    endif ()
  endforeach ()
endfunction ()


# ! Project targets.
# ========================================

add_subdirectory ("External")
add_subdirectory ("Common")
add_subdirectory ("ConvoSniffer")
add_subdirectory ("GPUCrusher")
add_subdirectory ("TextureOverride")

# set_property (DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "TextureOverride-LE3")

set (REPO_SOURCES
  .gitignore
  .gitmodules
  CMakeLists.txt
  CMakePresets.json
)

add_custom_target (Repository SOURCES ${REPO_SOURCES})
source_group (TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${REPO_SOURCES})
