# Shared warning/hardening flags. Apply with july_set_target_options(<target>).

function(july_set_target_options target)
    target_compile_options(${target} PRIVATE
        /W4 /WX /permissive- /utf-8 /Zc:__cplusplus /Zc:inline /sdl /guard:cf
        $<$<NOT:$<CONFIG:Debug>>:/Gy /Gw>)
    target_compile_definitions(${target} PRIVATE
        UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN
        _WIN32_WINNT=0x0A00 WINVER=0x0A00
        $<$<BOOL:${ENABLE_DIAGNOSTICS}>:JULY_DIAGNOSTICS=1>)
    # Needs the "MSVC Spectre-mitigated libs" VS component; mandatory for release (Phase 12).
    if(JULY_SPECTRE AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE /Qspectre)
    endif()
    target_link_options(${target} PRIVATE
        /guard:cf /DYNAMICBASE /NXCOMPAT
        $<$<NOT:$<CONFIG:Debug>>:/OPT:REF /OPT:ICF>)
endfunction()
