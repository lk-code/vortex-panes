# vp_settings: settings window, WinUI 3 hosted in a Win32 window via XAML Islands.
# WinUI is initialized on first use; nothing of the Windows App SDK loads before that.
add_library(vp_settings STATIC
    settings_log.h
    settings_window.h
    settings_window.cpp
    winrt_pch.h
    xaml_runtime.h
    xaml_runtime.cpp
)
target_link_libraries(vp_settings
    PUBLIC vp_core
    PRIVATE vp_winappsdk comctl32 dwmapi shcore shell32 user32 gdi32
)
# The C++/WinRT headers are large; precompiling them keeps rebuilds fast.
target_precompile_headers(vp_settings PRIVATE winrt_pch.h)
vp_target_defaults(vp_settings)
