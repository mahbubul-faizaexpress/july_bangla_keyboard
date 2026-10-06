add_executable(JulyBangla WIN32
    Main.cpp
    ModeSync.cpp
    ModeVisuals.cpp
    StatusBar.cpp
    TrayIcon.cpp
    "${PROJECT_SOURCE_DIR}/resources/JulyBangla.manifest")

target_link_libraries(JulyBangla PRIVATE july_engine july_common ole32 oleaut32 shell32 dwmapi)
july_set_target_options(JulyBangla)
