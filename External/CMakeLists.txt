# ! Submodule: LExSDKv2.
# ========================================

set (LESDK_WITH_LE1 ON)
set (LESDK_WITH_LE2 ON)
set (LESDK_WITH_LE3 ON)
set (LESDK_WITH_SAMPLES OFF)
set (LESDK_WITH_TESTS OFF)

add_subdirectory ("LExSDKv2")
set_target_properties (LESDK-LE1 PROPERTIES FOLDER "External")
set_target_properties (LESDK-LE2 PROPERTIES FOLDER "External")
set_target_properties (LESDK-LE3 PROPERTIES FOLDER "External")


# ! Submodule: simdutf.
# ========================================

set (SIMDUTF_TESTS OFF)
set (SIMDUTF_TOOLS OFF)
set (SIMDUTF_ICONV OFF)

add_subdirectory ("simdutf")
set_target_properties (simdutf PROPERTIES FOLDER "External")
set_target_properties (simdutf-singleheader-files PROPERTIES FOLDER "External")


# ! Submodule: spdlog.
# ========================================

set (SPDLOG_USE_STD_FORMAT ON)
set (SPDLOG_WCHAR_SUPPORT ON)

add_subdirectory ("spdlog")
set_target_properties (spdlog PROPERTIES FOLDER "External")
