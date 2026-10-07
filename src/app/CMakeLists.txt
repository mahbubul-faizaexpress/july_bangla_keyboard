add_executable(JulyBangla WIN32
    Main.cpp
    ModeSync.cpp
    Splash.cpp
    StatusBar.cpp
    TrayIcon.cpp
    JulyBangla.rc
    "${PROJECT_SOURCE_DIR}/resources/JulyBangla.manifest")

target_include_directories(JulyBangla PRIVATE "${PROJECT_BINARY_DIR}/generated")
target_link_libraries(JulyBangla PRIVATE july_engine july_common ole32 oleaut32 shell32 dwmapi windowscodecs msimg32)
# The splash artwork is embedded as a resource: rebuild the .rc when the image changes.
set_property(SOURCE JulyBangla.rc APPEND PROPERTY OBJECT_DEPENDS "${PROJECT_SOURCE_DIR}/resources/splash.jpg")
july_set_target_options(JulyBangla)
