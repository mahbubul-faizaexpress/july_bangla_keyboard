add_executable(july_unit_tests
    TestMain.cpp
    unit/ClassicTests.cpp
    unit/ComposerTests.cpp
    unit/InputModeTests.cpp
    unit/LayoutTests.cpp
    unit/RingBufferTests.cpp)

target_include_directories(july_unit_tests PRIVATE .)
target_compile_definitions(july_unit_tests PRIVATE
    JULY_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden")
target_link_libraries(july_unit_tests PRIVATE july_engine july_common)
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

# classicgen validation, same pattern as layoutgen.
set(CLASSICGEN_CASES "${CMAKE_CURRENT_SOURCE_DIR}/classicgen")
set(CLASSICGEN_OUT "${CMAKE_CURRENT_BINARY_DIR}/classicgen-out.inc")
file(GLOB bad_classic CONFIGURE_DEPENDS "${CLASSICGEN_CASES}/bad_*.classic")
foreach(table IN LISTS bad_classic)
    get_filename_component(name "${table}" NAME_WE)
    add_test(NAME classicgen_rejects_${name} COMMAND july_classicgen "${table}" "${CLASSICGEN_OUT}")
    set_tests_properties(classicgen_rejects_${name} PROPERTIES PASS_REGULAR_EXPRESSION "\\): error: ")
endforeach()
add_test(NAME classicgen_accepts_good_minimal
    COMMAND july_classicgen "${CLASSICGEN_CASES}/good_minimal.classic" "${CLASSICGEN_OUT}")
add_test(NAME classicgen_strict_rejects_converter_row
    COMMAND july_classicgen "${CLASSICGEN_CASES}/strict_converter.classic" "${CLASSICGEN_OUT}" --strict)
set_tests_properties(classicgen_strict_rejects_converter_row PROPERTIES
    PASS_REGULAR_EXPRESSION "rejected in --strict mode")

# End-to-end smoke test through real TSF. Needs the text service registered
# (tools/dev/Register-Dev.ps1), so it is built but not part of the default ctest run.
if(NOT JULY_ARCH_X86)
    add_executable(july_tip_smoke integration/TipSmoke.cpp)
    target_link_libraries(july_tip_smoke PRIVATE ole32 oleaut32 user32 version)
    july_set_target_options(july_tip_smoke)
endif()
