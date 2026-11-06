# vp_core: platform-independent logic. Must not include <windows.h>.

# Application metadata header (kAppName, kDeveloper, kWebsite, kRepository, kVersion)
configure_file(app_info.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/core/app_info.h @ONLY)

add_library(vp_core STATIC
    app_info.h.in
    build_info.h
    geometry.h
    hresult.h
    hresult.cpp
    unique_resource.h
)
target_include_directories(vp_core PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)
vp_target_defaults(vp_core)
