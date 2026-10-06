add_executable(july_unit_tests
    TestMain.cpp
    unit/ComposerTests.cpp
    unit/InputModeTests.cpp
    unit/LayoutTests.cpp
    unit/RingBufferTests.cpp)

target_include_directories(july_unit_tests PRIVATE .)
target_compile_definitions(july_unit_tests PRIVATE
    JULY_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden")
target_link_libraries(july_unit_tests PRIVATE july_engine)
july_set_target_options(july_unit_tests)

add_test(NAME july_unit_tests COMMAND july_unit_tests)

add_executable(july_fuzz fuzz/ComposerFuzz.cpp)
target_link_libraries(july_fuzz PRIVATE july_engine)
july_set_target_options(july_fuzz)
foreach(seed IN ITEMS 1 20261006 987654321)
    add_test(NAME july_fuzz_seed_${seed} COMMAND july_fuzz ${seed} 1000000)
endforeach()

# layoutgen validation: every bad_*.layout must be rejected; good ones must pass.
set(LAYOUTGEN_CASES "${CMAKE_CURRENT_SOURCE_DIR}/layoutgen")
set(LAYOUTGEN_OUT "${CMAKE_CURRENT_BINARY_DIR}/layoutgen-out.inc")
file(GLOB bad_layouts CONFIGURE_DEPENDS "${LAYOUTGEN_CASES}/bad_*.layout")
foreach(layout IN LISTS bad_layouts)
    get_filename_component(name "${layout}" NAME_WE)
    add_test(NAME layoutgen_rejects_${name} COMMAND july_layoutgen "${layout}" "${LAYOUTGEN_OUT}")
    # Require a diagnostic, not just a non-zero exit, so a crash cannot pass as a rejection.
    set_tests_properties(layoutgen_rejects_${name} PROPERTIES PASS_REGULAR_EXPRESSION "\\): error: ")
endforeach()

add_test(NAME layoutgen_accepts_good_minimal
    COMMAND july_layoutgen "${LAYOUTGEN_CASES}/good_minimal.layout" "${LAYOUTGEN_OUT}")
add_test(NAME layoutgen_accepts_memory_row_by_default
    COMMAND july_layoutgen "${LAYOUTGEN_CASES}/strict_memory.layout" "${LAYOUTGEN_OUT}")
add_test(NAME layoutgen_strict_rejects_memory_row
    COMMAND july_layoutgen "${LAYOUTGEN_CASES}/strict_memory.layout" "${LAYOUTGEN_OUT}" --strict)
set_tests_properties(layoutgen_strict_rejects_memory_row PROPERTIES
    PASS_REGULAR_EXPRESSION "rejected in --strict mode")
