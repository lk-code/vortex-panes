# VortexPanes.exe: entry point, main loop, notification area icon.

set(VP_RESOURCE_DIR ${PROJECT_SOURCE_DIR}/resources)
configure_file(${VP_RESOURCE_DIR}/app.rc.in ${CMAKE_CURRENT_BINARY_DIR}/app.rc @ONLY)

add_executable(VortexPanes WIN32
    main.cpp
    application.h
    application.cpp
    debug_log.h
    debug_log.cpp
    localization.h
    localization.cpp
    single_instance.h
    single_instance.cpp
    tray_icon.h
    tray_icon.cpp
    win32_handles.h
    ${CMAKE_CURRENT_BINARY_DIR}/app.rc
    ${VP_RESOURCE_DIR}/app.manifest
    ${VP_RESOURCE_DIR}/resource.h
)
# resources/ provides resource.h and strings/*.rc to both the compiler and rc.exe.
target_include_directories(VortexPanes PRIVATE ${VP_RESOURCE_DIR})
target_link_libraries(VortexPanes PRIVATE
    vp_settings
    vp_render
    vp_platform
    vp_core
    user32
    shell32
    advapi32
    comctl32
)
vp_target_defaults(VortexPanes)
# Windows App SDK runtime next to the executable (self-contained) and the merged resource index.
vp_winappsdk_deploy(VortexPanes)

install(TARGETS VortexPanes RUNTIME DESTINATION .)
