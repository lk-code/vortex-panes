# vp_render: D3D11 device, DirectComposition, one swap chain per monitor, pipelines.

# --- fxc from the Windows SDK selected by the Visual Studio generator -----------
if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
    set(_vp_sdk_version ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
else()
    set(_vp_sdk_version ${CMAKE_SYSTEM_VERSION})
endif()
set(_vp_sdk_bin "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/${_vp_sdk_version}")
# CMAKE_HOST_SYSTEM_PROCESSOR is AMD64 or ARM64; the SDK folders are x64 and arm64.
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
    set(_vp_host_arch arm64)
else()
    set(_vp_host_arch x64)
endif()
find_program(FXC_EXECUTABLE fxc
    HINTS "${_vp_sdk_bin}/${_vp_host_arch}" "${_vp_sdk_bin}/arm64" "${_vp_sdk_bin}/x64" "${_vp_sdk_bin}/x86"
    REQUIRED
    NO_DEFAULT_PATH
)
message(STATUS "fxc: ${FXC_EXECUTABLE}")

# Compiles an HLSL file into a header with a BYTE array named var_name.
# DXBC bytecode is architecture-independent.
set(VP_SHADER_DIR ${PROJECT_SOURCE_DIR}/shaders)
set(VP_SHADER_OUT ${CMAKE_CURRENT_BINARY_DIR}/shaders)
function(vp_compile_shader out_header name profile var_name)
    set(source ${VP_SHADER_DIR}/${name}.hlsl)
    set(header ${VP_SHADER_OUT}/${name}.h)
    add_custom_command(
        OUTPUT ${header}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${VP_SHADER_OUT}
        COMMAND ${FXC_EXECUTABLE} /nologo /O3 /WX /Qstrip_reflect /Qstrip_debug /T ${profile} /E main
                /Vn ${var_name} /Fh ${header} ${source}
        MAIN_DEPENDENCY ${source}
        COMMENT "fxc ${name}.hlsl (${profile})"
        VERBATIM
    )
    set(${out_header} ${header} PARENT_SCOPE)
endfunction()

vp_compile_shader(shape_vs_header shape_vs vs_4_0 g_shapeVS)
vp_compile_shader(shape_ps_header shape_ps ps_4_0 g_shapePS)

add_library(vp_render STATIC
    graphics_device.h
    graphics_device.cpp
    monitor_overlay.h
    monitor_overlay.cpp
    renderer.h
    renderer.cpp
    shaders.h
    shaders.cpp
    ${VP_SHADER_DIR}/shape_vs.hlsl
    ${VP_SHADER_DIR}/shape_ps.hlsl
    ${shape_vs_header}
    ${shape_ps_header}
)
# The .hlsl files carry the custom command (MAIN_DEPENDENCY), so MSBuild's own HLSL
# compiler is not used for them.
target_include_directories(vp_render PRIVATE ${VP_SHADER_OUT})
target_link_libraries(vp_render PUBLIC vp_core PRIVATE d3d11 dxgi dcomp user32)
vp_target_defaults(vp_render)
