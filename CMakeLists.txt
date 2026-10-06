cmake_minimum_required(VERSION 3.25)

project(JulyBanglaKeyboard
    VERSION 0.1.0
    DESCRIPTION "Native Windows Bangla input method (TSF)"
    LANGUAGES CXX)

if(NOT MSVC)
    message(FATAL_ERROR "July Bangla Keyboard targets MSVC or clang-cl on Windows.")
endif()

option(BUILD_TESTS            "Build unit tests"                   ON)
option(BUILD_BENCHMARKS       "Build keyboard_benchmark"           ON)
option(STRICT_LAYOUT          "Reject unconfirmed layout rows (release)" OFF)
option(BUILD_COMPANION       "Build the tray/status companion"    ON)
option(ENABLE_DIAGNOSTICS     "Compile diagnostic instrumentation" OFF)
option(JULY_SPECTRE           "Compile with /Qspectre (needs Spectre-mitigated libs)" OFF)

# Independent version tracks (spec §48).
set(JULY_ENGINE_VERSION "0.1.0")
set(JULY_LAYOUT_VERSION "0.1.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static CRT everywhere: the TIP is loaded into arbitrary host processes and must not
# depend on a VC++ redistributable being present.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(JulyCompilerOptions)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(JULY_ARCH_X86 ON)
endif()

add_subdirectory(tools/layoutgen)  # always built: the engine's tables are generated
add_subdirectory(src/engine)
add_subdirectory(src/common)
add_subdirectory(src/tip)

# The companion, tests and benchmark are x64-only products; x86 builds only need the TIP.
if(NOT JULY_ARCH_X86)
    if(BUILD_COMPANION)
        add_subdirectory(src/app)
    endif()
    if(BUILD_BENCHMARKS)
        add_subdirectory(bench)
    endif()
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
