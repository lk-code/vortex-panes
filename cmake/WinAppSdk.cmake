# Windows App SDK (WinUI 3) and C++/WinRT for the settings window (XAML Islands).
#
# - Packages come from nuget.org with fixed versions and SHA256 hashes. They are downloaded and
#   extracted at configure time into VP_NUGET_CACHE_DIR, which all presets share.
# - The C++/WinRT projection headers are generated with cppwinrt.exe at configure time.
# - Deployment uses the MSBuild logic of the Windows App SDK packages, imported into the generated
#   .vcxproj of the executable (VS_PROJECT_IMPORT). See vp_winappsdk_deploy().
#
# Only the components the settings window needs are referenced (WinUI and its dependencies), not
# the Microsoft.WindowsAppSDK metapackage with its AI and machine learning components.
include_guard(GLOBAL)

option(VP_WINAPPSDK_SELF_CONTAINED
    "Deploy the Windows App SDK runtime next to the executable (portable zip). OFF: framework-dependent (MSIX)" ON)
set(VP_NUGET_CACHE_DIR "${PROJECT_SOURCE_DIR}/out/nuget" CACHE PATH
    "Download and extraction cache for NuGet packages (shared between presets)")
set(VP_NUGET_FEED "https://api.nuget.org/v3-flatcontainer" CACHE STRING "NuGet flat container base URL")

# --- Pinned packages: id | version | SHA256 of the .nupkg ------------------------------------
# When updating, take the dependency versions from the .nuspec files (NuGet resolves the lowest
# matching version) and verify the hashes against the packageHash (SHA512) in the nuget.org catalog.
set(VP_WINAPPSDK_VERSION 2.6.5)
set(_vp_nuget_packages
    "Microsoft.Windows.CppWinRT|3.0.260818.1|a8993608ae9263a8288e1a1d9ee2684a5632f4acc59e1e89ddd86b3b889a7367"
    "Microsoft.WindowsAppSDK.Base|${VP_WINAPPSDK_VERSION}|cb9d3c4fb9cd98907709c95832a642cfb0331fd8e69df649f662e96b09b8cb48"
    "Microsoft.WindowsAppSDK.Foundation|${VP_WINAPPSDK_VERSION}|548033e51be965804f7119e1936fe7f78f324d668becb731fcacb101f3bb18e4"
    "Microsoft.WindowsAppSDK.InteractiveExperiences|${VP_WINAPPSDK_VERSION}|3782df5d796dfe872175da0db2b64b616611532fd14b06d9e2102895286df136"
    "Microsoft.WindowsAppSDK.WinUI|${VP_WINAPPSDK_VERSION}|8cd902f8abaf4ac0f75bdd187fd420eb98419818f39ab5e853ed126bc26a6af9"
    # Build only: the WinUI metadata references the WebView2 types.
    "Microsoft.Web.WebView2|1.0.3719.77|2f6be3a10a1a8d6d1fde986af4131dab344f8181fffe75590824e0f4b037ed73"
)
if(NOT VP_WINAPPSDK_SELF_CONTAINED)
    # Framework-dependent: declares the dependency on the Windows App Runtime framework package.
    list(APPEND _vp_nuget_packages
        "Microsoft.WindowsAppSDK.Runtime|${VP_WINAPPSDK_VERSION}|b18d0d0ecae0c62230dc70fcc30f843d3f3a49c806f861d2b1990b2f01ee6352")
endif()

# Downloads (if needed) and extracts one package. Sets <out_var> to the extraction directory.
function(vp_nuget_package out_var id version sha256)
    string(TOLOWER "${id}" lid)
    set(dir "${VP_NUGET_CACHE_DIR}/${lid}.${version}")
    set(marker "${dir}/.vp-sha256")
    set(cached "")
    if(EXISTS "${marker}")
        file(READ "${marker}" cached)
    endif()
    if(NOT cached STREQUAL sha256)
        set(archive "${VP_NUGET_CACHE_DIR}/${lid}.${version}.nupkg")
        message(STATUS "NuGet: downloading ${id} ${version}")
        file(DOWNLOAD "${VP_NUGET_FEED}/${lid}/${version}/${lid}.${version}.nupkg" "${archive}"
            EXPECTED_HASH SHA256=${sha256} TLS_VERIFY ON STATUS status)
        list(GET status 0 code)
        if(NOT code EQUAL 0)
            list(GET status 1 reason)
            file(REMOVE "${archive}")
            message(FATAL_ERROR "NuGet: download of ${id} ${version} failed: ${reason}")
        endif()
        file(REMOVE_RECURSE "${dir}")
        file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${dir}")
        file(REMOVE "${archive}")
        file(WRITE "${marker}" "${sha256}")
    endif()
    set(${out_var} "${dir}" PARENT_SCOPE)
endfunction()

foreach(entry IN LISTS _vp_nuget_packages)
    string(REPLACE "|" ";" parts "${entry}")
    list(GET parts 0 id)
    list(GET parts 1 version)
    list(GET parts 2 sha256)
    vp_nuget_package(dir ${id} ${version} ${sha256})
    string(REPLACE "." "_" var "${id}")
    set(VP_PKG_${var} "${dir}")
endforeach()

# --- C++/WinRT projection ----------------------------------------------------------------------
set(VP_CPPWINRT_DIR "${CMAKE_BINARY_DIR}/cppwinrt")
file(GLOB _foundation_winmds "${VP_PKG_Microsoft_WindowsAppSDK_Foundation}/metadata/*.winmd")
file(GLOB _ix_winmds "${VP_PKG_Microsoft_WindowsAppSDK_InteractiveExperiences}/metadata/10.0.18362.0/*.winmd")
file(GLOB _winui_winmds "${VP_PKG_Microsoft_WindowsAppSDK_WinUI}/metadata/*.winmd")
set(_vp_winmds
    ${_foundation_winmds} ${_ix_winmds} ${_winui_winmds}
    "${VP_PKG_Microsoft_Web_WebView2}/lib/Microsoft.Web.WebView2.Core.winmd"
)

set(_cppwinrt_exe "${VP_PKG_Microsoft_Windows_CppWinRT}/bin/cppwinrt.exe")
string(SHA256 _projection_key "${_cppwinrt_exe};${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION};${_vp_winmds}")
set(_projection_stamp "${VP_CPPWINRT_DIR}/.vp-projection")
set(_projection_done "")
if(EXISTS "${_projection_stamp}")
    file(READ "${_projection_stamp}" _projection_done)
endif()
if(NOT _projection_done STREQUAL _projection_key)
    message(STATUS "C++/WinRT: generating projection (Windows SDK ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})")
    file(REMOVE_RECURSE "${VP_CPPWINRT_DIR}")
    set(_args -input ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
    foreach(winmd IN LISTS _vp_winmds)
        list(APPEND _args -input "${winmd}")
    endforeach()
    execute_process(COMMAND "${_cppwinrt_exe}" ${_args} -output "${VP_CPPWINRT_DIR}"
        RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "cppwinrt.exe failed (${result}):\n${out}\n${err}")
    endif()
    file(WRITE "${_projection_stamp}" "${_projection_key}")
endif()

# Interface target for code that uses WinUI: projection headers and the native interop headers of
# the Windows App SDK, both as SYSTEM includes so that /external:W0 applies.
add_library(vp_winappsdk INTERFACE)
target_include_directories(vp_winappsdk SYSTEM INTERFACE
    "${VP_CPPWINRT_DIR}"
    "${VP_PKG_Microsoft_WindowsAppSDK_InteractiveExperiences}/include"
)
target_compile_options(vp_winappsdk INTERFACE /bigobj)
# Ro* functions; everything else C++/WinRT needs comes from the default Windows libraries.
target_link_libraries(vp_winappsdk INTERFACE runtimeobject)

# --- Deployment (executable only) --------------------------------------------------------------
# Imports the MSBuild logic of the packages into the executable's .vcxproj. All auto-initializers
# are off because they would load the Windows App Runtime at process start; the settings module
# initializes WinUI on first use. Activation works through the registration-free WinRT entries
# that the self-contained targets merge into the application manifest.
function(vp_winappsdk_deploy target)
    set(components Microsoft_WindowsAppSDK_Base Microsoft_WindowsAppSDK_InteractiveExperiences
                   Microsoft_WindowsAppSDK_Foundation Microsoft_WindowsAppSDK_WinUI)
    if(NOT VP_WINAPPSDK_SELF_CONTAINED)
        list(APPEND components Microsoft_WindowsAppSDK_Runtime)
    endif()
    set(imports "")
    foreach(ext props targets)
        foreach(pkg IN LISTS components)
            string(REPLACE "_" "." id "${pkg}")
            list(APPEND imports "${VP_PKG_${pkg}}/build/native/${id}.${ext}")
        endforeach()
    endforeach()
    # Our own adjustments come last (see winappsdk_overrides.targets).
    list(APPEND imports "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/winappsdk_overrides.targets")

    if(VP_WINAPPSDK_SELF_CONTAINED)
        set(self_contained true)
    else()
        set(self_contained false)
    endif()
    set_target_properties(${target} PROPERTIES
        VS_PROJECT_IMPORT "${imports}"
        VS_GLOBAL_WindowsPackageType None
        VS_GLOBAL_WindowsAppSDKSelfContained ${self_contained}
        VS_GLOBAL_WindowsAppSdkBootstrapInitialize false
        VS_GLOBAL_WindowsAppSdkUndockedRegFreeWinRTInitialize false
        VS_GLOBAL_WindowsAppSdkDeploymentManagerInitialize false
    )

    # License terms of the Windows App SDK; its binaries are redistributed under these terms.
    install(FILES "${VP_PKG_Microsoft_WindowsAppSDK_WinUI}/license.txt"
        DESTINATION licenses RENAME Microsoft.WindowsAppSDK-LICENSE.txt)
    install(FILES "${VP_PKG_Microsoft_WindowsAppSDK_Base}/NOTICE.txt"
        DESTINATION licenses RENAME Microsoft.WindowsAppSDK-NOTICE.txt)
    install(FILES "${VP_PKG_Microsoft_WindowsAppSDK_WinUI}/NOTICE.txt"
        DESTINATION licenses RENAME Microsoft.WindowsAppSDK.WinUI-NOTICE.txt)

    if(NOT VP_WINAPPSDK_SELF_CONTAINED)
        return()
    endif()

    # --- Merged resource index (<exe>.pri) -------------------------------------------------
    # Unpackaged WinUI looks up its theme resources (ms-appx:///Microsoft.UI.Xaml/Themes/...) in
    # the resource index of the application. The MSIX tooling would generate it; here makepri.exe
    # from the Windows SDK merges the PRI files of the components.
    _vp_winappsdk_arch(arch)
    set(native_dirs "")
    foreach(pkg Microsoft_WindowsAppSDK_Foundation Microsoft_WindowsAppSDK_InteractiveExperiences
                Microsoft_WindowsAppSDK_WinUI)
        list(APPEND native_dirs "${VP_PKG_${pkg}}/runtimes-framework/win-${arch}/native")
    endforeach()

    set(pri_stage "${CMAKE_CURRENT_BINARY_DIR}/winappsdk-pri")
    file(MAKE_DIRECTORY "${pri_stage}")
    set(pri_inputs "")
    set(resfiles "")
    foreach(dir IN LISTS native_dirs)
        file(GLOB pris "${dir}/*.pri")
        foreach(pri IN LISTS pris)
            get_filename_component(name "${pri}" NAME)
            configure_file("${pri}" "${pri_stage}/${name}" COPYONLY)
            list(APPEND pri_inputs "${pri}")
            string(APPEND resfiles "${name}\n")
        endforeach()
    endforeach()
    file(WRITE "${pri_stage}/pri.resfiles" "${resfiles}")
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/priconfig.xml" "${pri_stage}/priconfig.xml" COPYONLY)

    _vp_find_sdk_tool(makepri VP_MAKEPRI_EXECUTABLE)
    set(merged_pri "${CMAKE_CURRENT_BINARY_DIR}/winappsdk/${target}.pri")
    add_custom_command(OUTPUT "${merged_pri}"
        COMMAND "${VP_MAKEPRI_EXECUTABLE}" new /pr "${pri_stage}" /cf "${pri_stage}/priconfig.xml"
                /of "${merged_pri}" /o > "${CMAKE_CURRENT_BINARY_DIR}/winappsdk/makepri.log"
        DEPENDS ${pri_inputs} "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/priconfig.xml"
        COMMENT "Merging Windows App SDK resource indexes into ${target}.pri"
        VERBATIM)
    target_sources(${target} PRIVATE "${merged_pri}")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${merged_pri}" "$<TARGET_FILE_DIR:${target}>"
        VERBATIM)

    # --- install(): the runtime files the self-contained targets copy next to the executable --
    # Same patterns as AddMicrosoftWindowsAppSDKPayloadFilesFromComponents in
    # Microsoft.WindowsAppSDK.SelfContained.targets, without the .winmd metadata (not needed at
    # run time by C++ code).
    foreach(dir IN LISTS native_dirs)
        install(DIRECTORY "${dir}/" DESTINATION .
            FILES_MATCHING
                PATTERN "*.dll" PATTERN "*.mui" PATTERN "*.png" PATTERN "*.xaml" PATTERN "*.xbf"
                PATTERN "*.pri" PATTERN "workloads*.json" PATTERN "restartAgent.exe" PATTERN "map.html")
    endforeach()
    install(FILES "${merged_pri}" DESTINATION .)
endfunction()

# Windows App SDK folder name of the target architecture (x86, x64, arm64).
function(_vp_winappsdk_arch out_var)
    if(CMAKE_VS_PLATFORM_NAME STREQUAL "Win32")
        set(${out_var} x86 PARENT_SCOPE)
    elseif(CMAKE_VS_PLATFORM_NAME STREQUAL "ARM64")
        set(${out_var} arm64 PARENT_SCOPE)
    else()
        set(${out_var} x64 PARENT_SCOPE)
    endif()
endfunction()

# Finds a host tool in the bin folder of the Windows SDK selected by the Visual Studio generator.
function(_vp_find_sdk_tool name out_var)
    set(sdk_bin "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
        set(host arm64)
    else()
        set(host x64)
    endif()
    find_program(${out_var} ${name}
        HINTS "${sdk_bin}/${host}" "${sdk_bin}/x64" "${sdk_bin}/x86"
        REQUIRED NO_DEFAULT_PATH)
endfunction()
