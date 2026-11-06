# vp_core: platform-independent logic. Must not include <windows.h>.
add_library(vp_core STATIC
    geometry.h
    unique_resource.h
    build_info.h
    hresult.h
    hresult.cpp
)
target_include_directories(vp_core PUBLIC ${PROJECT_SOURCE_DIR}/src)
vp_target_defaults(vp_core)
