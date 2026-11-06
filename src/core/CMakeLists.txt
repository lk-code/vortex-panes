# vp_core: platform-independent logic. Must not include <windows.h>.

# Application metadata header (kAppName, kDeveloper, kWebsite, kRepository, kVersion)
configure_file(app_info.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/core/app_info.h @ONLY)

add_library(vp_core STATIC
    app_info.h.in
    build_info.h
    command_line.h
    command_line.cpp
    fixed_timestep.h
    fixed_timestep.cpp
    flake.h
    flake_pool.cpp
    fps_counter.h
    fps_counter.cpp
    frame_pacer.h
    frame_pacer.cpp
    geometry.h
    hresult.h
    hresult.cpp
    notices_text.h
    notices_text.cpp
    pcg32.h
    snowfall.h
    snowfall.cpp
    string_block.h
    string_block.cpp
    test_scene.h
    test_scene.cpp
    text_format.h
    text_format.cpp
    tray_menu.h
    tray_menu.cpp
    tray_theme.h
    ui_language.h
    ui_language.cpp
    unique_resource.h
    wind.h
    wind.cpp
)
target_include_directories(vp_core PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)
vp_target_defaults(vp_core)
