# vp_platform: monitors now; window tracking, occlusion, cursor and full-screen detection later.
add_library(vp_platform STATIC
    monitors.h
    monitors.cpp
)
target_link_libraries(vp_platform PUBLIC vp_core PRIVATE user32)
vp_target_defaults(vp_platform)
