# Unit tests for pure logic. Discovery runs at test time (PRE_TEST) so that
# cross-compiled ARM64 builds do not need to execute the test binaries.

add_executable(core_tests
    core_tests/build_info_test.cpp
    core_tests/command_line_test.cpp
    core_tests/geometry_test.cpp
    core_tests/hresult_test.cpp
    core_tests/string_block_test.cpp
    core_tests/tray_menu_test.cpp
    core_tests/tray_theme_test.cpp
    core_tests/ui_language_test.cpp
    core_tests/unique_resource_test.cpp
)
target_link_libraries(core_tests PRIVATE vp_core GTest::gtest_main)
vp_target_defaults(core_tests)
gtest_discover_tests(core_tests DISCOVERY_MODE PRE_TEST)
