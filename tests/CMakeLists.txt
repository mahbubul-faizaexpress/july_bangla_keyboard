add_executable(july_unit_tests
    TestMain.cpp
    unit/InputModeTests.cpp
    unit/RingBufferTests.cpp)

target_include_directories(july_unit_tests PRIVATE .)
target_link_libraries(july_unit_tests PRIVATE july_engine)
july_set_target_options(july_unit_tests)

add_test(NAME july_unit_tests COMMAND july_unit_tests)
