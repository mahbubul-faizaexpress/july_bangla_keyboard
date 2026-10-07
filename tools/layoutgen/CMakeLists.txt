# Also usable as a standalone project: the Android build (and later Linux/macOS builds)
# compile these generators with the host compiler, because they run on the build machine.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.22)
    project(JulyHostTools LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    # One fixed output directory for single- and multi-config generators ($<0:> stops the
    # per-config subdirectory).
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin$<0:>")
    if(MSVC)
        add_compile_options(/W4 /WX /permissive- /utf-8)
    else()
        add_compile_options(-Wall -Wextra -Werror)
    endif()
    function(july_set_target_options target)
    endfunction()
endif()

add_executable(july_layoutgen LayoutGen.cpp)
july_set_target_options(july_layoutgen)

add_executable(july_classicgen ClassicGen.cpp)
july_set_target_options(july_classicgen)
