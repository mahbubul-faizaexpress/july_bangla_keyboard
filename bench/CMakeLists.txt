add_executable(keyboard_benchmark
    KeyboardBenchmark.cpp)

target_link_libraries(keyboard_benchmark PRIVATE july_engine)
july_set_target_options(keyboard_benchmark)
