# VortexPanes.exe: entry point, main loop, notification area icon.
add_executable(VortexPanes WIN32
    main.cpp
)
target_link_libraries(VortexPanes PRIVATE vp_core)
vp_target_defaults(VortexPanes)

install(TARGETS VortexPanes RUNTIME DESTINATION .)
