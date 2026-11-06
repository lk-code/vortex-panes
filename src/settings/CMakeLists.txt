# vp_settings: settings window. WinUI 3 via XAML Islands follows in M2; until then a stub.
add_library(vp_settings STATIC
    settings_window.h
    settings_window.cpp
)
target_link_libraries(vp_settings PUBLIC vp_core)
vp_target_defaults(vp_settings)
