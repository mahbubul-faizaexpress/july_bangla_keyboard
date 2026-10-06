add_library(JulyTip SHARED
    DllMain.cpp
    JulyTip.def)

target_link_libraries(JulyTip PRIVATE july_engine)
july_set_target_options(JulyTip)
