# vp_settings: settings window, WinUI 3 hosted in a Win32 window via XAML Islands.
# WinUI is initialized on first use; nothing of the Windows App SDK loads before that.
add_library(vp_settings STATIC
    settings_window.h
    settings_window.cpp
)
target_link_libraries(vp_settings
    PUBLIC vp_core
    PRIVATE vp_winappsdk
)
vp_target_defaults(vp_settings)
