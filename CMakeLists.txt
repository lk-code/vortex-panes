cmake_minimum_required(VERSION 3.25)

project(VortexPanes VERSION 0.1.0 LANGUAGES CXX)

if(NOT MSVC)
    message(FATAL_ERROR "VortexPanes must be built with MSVC.")
endif()

# --- Application metadata (flows into app_info.h and VERSIONINFO) -------------
set(VP_APP_NAME "VortexPanes" CACHE STRING "Application name")
set(VP_DEVELOPER "lk-code" CACHE STRING "Developer and copyright holder")
set(VP_WEBSITE "https://www.lk-code.dev/" CACHE STRING "Website URL")
set(VP_REPOSITORY "https://github.com/lk-code/vortex-panes" CACHE STRING "Source repository URL")

option(VP_BUILD_TESTS "Build the unit tests" ON)

# --- Global settings (must precede all targets, including GoogleTest) ---------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static CRT for every target.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Executables land in bin/<Config>/ with the Visual Studio generator.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# All .rc files are UTF-8.
string(APPEND CMAKE_RC_FLAGS " /nologo /c65001")

# --- Project-wide options for our own targets (not applied to GoogleTest) -----
add_library(vp_options INTERFACE)
target_compile_options(vp_options INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:/W4 /WX /permissive- /utf-8 /Zc:__cplusplus /external:anglebrackets /external:W0 /MP>
)
target_compile_definitions(vp_options INTERFACE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WIN32_WINNT=0x0A00
    WINVER=0x0A00
    $<$<CONFIG:Debug>:VP_DEBUG=1>
)
target_link_options(vp_options INTERFACE
    $<$<CONFIG:Release>:/OPT:REF /OPT:ICF>
)

# Applies the project options and whole-program optimization (/GL, /LTCG) in Release.
function(vp_target_defaults target)
    target_link_libraries(${target} PRIVATE vp_options)
    set_target_properties(${target} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endfunction()

# --- Modules ------------------------------------------------------------------
add_subdirectory(src/core)
add_subdirectory(src/platform)
add_subdirectory(src/render)
add_subdirectory(src/settings)
add_subdirectory(src/app)

# --- Tests --------------------------------------------------------------------
if(VP_BUILD_TESTS)
    include(FetchContent)
    set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz
        URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    include(GoogleTest)
    add_subdirectory(tests)
endif()
