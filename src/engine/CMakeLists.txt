set(JULY_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(Version.h.in "${JULY_GENERATED_DIR}/july/engine/Version.h" @ONLY)

set(JULY_BIJOY_LAYOUT "${PROJECT_SOURCE_DIR}/layouts/bijoy.layout")
set(JULY_BIJOY_INC "${JULY_GENERATED_DIR}/july/engine/generated/BijoyLayout.inc")
set(JULY_LAYOUTGEN_FLAGS "$<$<BOOL:${STRICT_LAYOUT}>:--strict>")

add_custom_command(
    OUTPUT "${JULY_BIJOY_INC}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${JULY_GENERATED_DIR}/july/engine/generated"
    COMMAND july_layoutgen "${JULY_BIJOY_LAYOUT}" "${JULY_BIJOY_INC}" ${JULY_LAYOUTGEN_FLAGS}
    DEPENDS july_layoutgen "${JULY_BIJOY_LAYOUT}"
    COMMENT "Generating Bijoy layout table"
    COMMAND_EXPAND_LISTS
    VERBATIM)

add_library(july_engine STATIC
    src/Composer.cpp
    src/InputMode.cpp
    src/Layout.cpp
    src/UnicodeBackend.cpp
    "${JULY_BIJOY_INC}")

target_include_directories(july_engine PUBLIC
    include
    "${JULY_GENERATED_DIR}")

july_set_target_options(july_engine)
