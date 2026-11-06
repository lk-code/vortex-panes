# Unit tests for pure logic. Discovery runs at test time (PRE_TEST) so that
# cross-compiled ARM64 builds do not need to execute the test binaries.

add_executable(core_tests
    core_tests/build_info_test.cpp
    core_tests/command_line_test.cpp
    core_tests/fixed_timestep_test.cpp
    core_tests/flake_pool_test.cpp
    core_tests/fps_counter_test.cpp
    core_tests/frame_pacer_test.cpp
    core_tests/geometry_test.cpp
    core_tests/hresult_test.cpp
    core_tests/notices_text_test.cpp
    core_tests/pcg32_test.cpp
    core_tests/snowfall_test.cpp
    core_tests/string_block_test.cpp
    core_tests/test_scene_test.cpp
    core_tests/text_format_test.cpp
    core_tests/tray_menu_test.cpp
    core_tests/tray_theme_test.cpp
    core_tests/ui_language_test.cpp
    core_tests/unique_resource_test.cpp
    core_tests/wind_test.cpp
)
target_link_libraries(core_tests PRIVATE vp_core GTest::gtest_main)
vp_target_defaults(core_tests)
gtest_discover_tests(core_tests DISCOVERY_MODE PRE_TEST)

add_executable(platform_tests
    platform_tests/monitors_test.cpp
)
target_link_libraries(platform_tests PRIVATE vp_platform GTest::gtest_main)
vp_target_defaults(platform_tests)
gtest_discover_tests(platform_tests DISCOVERY_MODE PRE_TEST)
