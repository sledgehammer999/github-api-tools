cmake_minimum_required(VERSION 3.18 FATAL_ERROR) # Policies <= CMP0108 are deprecated

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

project(GitHub-API-Tools
    VERSION 1.0
    DESCRIPTION "This is a collection of helpful tools that operate using the GitHuB APIs."
    HOMEPAGE_URL "https://github.com/sledgehammer999/github-api-tools"
    LANGUAGES CXX
)

# use CONFIG mode first in find_package
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
# treat value specified by the CXX_STANDARD target property as a requirement by default
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# export compilation database for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# version requirements
set(requiredBoostVersion 1.74)
set(requiredOpenSSLVersion 1.1.1)

# features
include(FeatureSummary)
include(FeatureOptionsSetup)
feature_option(GH_API_TOOLS_VERBOSE_CONFIGURE "Show information about PACKAGES_FOUND and PACKAGES_NOT_FOUND in the configure output (only useful for debugging the CMake build scripts)" OFF)

if (MSVC)
    message(FATAL_ERROR "Building with MSVC is currently not supported. To build on Windows, use MinGW64")
endif()

# MingGW: link statically to everything (-static is also passed to the linker for each executable)
if (MINGW)
    set(Boost_USE_STATIC_LIBS ON)
    set(OPENSSL_USE_STATIC_LIBS ON)
endif()

find_package(Threads REQUIRED)
find_package(Boost ${requiredBoostVersion} REQUIRED COMPONENTS program_options)
find_package(OpenSSL ${requiredOpenSSLVersion} REQUIRED)

include(GNUInstallDirs)

add_subdirectory(AmendTitleAndApplyLabel)
add_subdirectory(MassCloseOldIssues)

if (GH_API_TOOLS_VERBOSE_CONFIGURE)
    feature_summary(WHAT ALL)
else()
    feature_summary(WHAT ENABLED_FEATURES DISABLED_FEATURES)
endif()
