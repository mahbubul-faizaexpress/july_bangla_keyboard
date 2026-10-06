# Windows-specific cold-path helpers shared by the TIP, the companion and tools.
add_library(july_common STATIC
    ClassicDiagnostics.cpp)

target_include_directories(july_common PUBLIC .)
target_link_libraries(july_common PUBLIC gdi32 user32)
july_set_target_options(july_common)
