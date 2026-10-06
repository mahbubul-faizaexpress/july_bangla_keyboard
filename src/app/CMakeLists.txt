add_executable(JulyBangla WIN32
    Main.cpp
    "${PROJECT_SOURCE_DIR}/resources/JulyBangla.manifest")

target_link_libraries(JulyBangla PRIVATE july_engine)
july_set_target_options(JulyBangla)
