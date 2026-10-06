add_library(JulyTip SHARED
    DllMain.cpp
    Registration.cpp
    TextService.cpp
    JulyTip.def)

target_link_libraries(JulyTip PRIVATE july_engine july_common ole32 oleaut32 advapi32 user32)
july_set_target_options(JulyTip)
