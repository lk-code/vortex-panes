# VortexPanes.exe: entry point, main loop, notification area icon.

set(VP_RESOURCE_DIR ${PROJECT_SOURCE_DIR}/resources)
configure_file(${VP_RESOURCE_DIR}/app.rc.in ${CMAKE_CURRENT_BINARY_DIR}/app.rc @ONLY)

add_executable(VortexPanes WIN32
    main.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/app.rc
    ${VP_RESOURCE_DIR}/app.manifest
    ${VP_RESOURCE_DIR}/resource.h
)
# resources/ provides resource.h and strings/*.rc to both the compiler and rc.exe.
target_include_directories(VortexPanes PRIVATE ${VP_RESOURCE_DIR})
target_link_libraries(VortexPanes PRIVATE vp_core)
vp_target_defaults(VortexPanes)

install(TARGETS VortexPanes RUNTIME DESTINATION .)
