configure_file(Version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/july/engine/Version.h" @ONLY)

add_library(july_engine STATIC
    src/InputMode.cpp)

target_include_directories(july_engine PUBLIC
    include
    "${CMAKE_CURRENT_BINARY_DIR}/generated")

july_set_target_options(july_engine)
