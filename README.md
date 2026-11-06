# VortexPanes

<img src="resources/icons/app_1024.png" alt="VortexPanes" width="96" align="right">

[![CI](https://github.com/lk-code/vortex-panes/actions/workflows/ci.yml/badge.svg)](https://github.com/lk-code/vortex-panes/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/lk-code/vortex-panes)](https://github.com/lk-code/vortex-panes/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

VortexPanes is a weather animation for the Windows desktop. Snow and rain fall across all monitors and settle on the top edges of open windows and on the taskbar. The animation reacts to window movement and to the mouse pointer.

## Features

- Five weather modes: Light Snow, Snowstorm, Sun, Light Rain and Rainstorm
- Snow and water droplets settle on window edges and the taskbar
- Moving a window slowly carries the snow along; moving it quickly, resizing, minimizing or closing it makes the snow fall off
- Snow and droplets can be brushed off the edges with the mouse pointer
- In Sun mode, everything melts away within one minute while subtle rays of light shine in from the top of the screen
- Rain droplets refract a selectable background image like a lens; besides the bundled images, a custom image can be used
- The overlay passes all mouse clicks through, does not appear in the taskbar and pauses automatically while full-screen applications are running
- User interface available in English, German, Spanish, French, Chinese (Simplified), Japanese and Korean

## System Requirements

- Windows 11 version 21H2 or later; no additional runtime has to be installed
- Graphics adapter with Direct3D 10 or later (otherwise software rendering via WARP)
- x64, ARM64 or x86 processor

## Installation

The latest version is available on the [Releases](https://github.com/lk-code/vortex-panes/releases) page.

| File | Intended for |
|---|---|
| `VortexPanes-<version>-x64.zip` | Windows PCs with Intel or AMD processors |
| `VortexPanes-<version>-arm64.zip` | Windows devices with ARM processors, e.g. Snapdragon |
| `VortexPanes-<version>-x86.zip` | 32-bit installations of Windows |

Extract the entire archive and run `VortexPanes.exe`. No installation is required. The folder contains the runtime libraries of the Windows App SDK for the settings window, so all files must be kept together. These libraries are distributed under the Microsoft license terms included in the `licenses` folder.

The executables are not code-signed. Windows SmartScreen therefore displays a notice on first launch, which can be confirmed via "More info" and "Run anyway". The checksums of all archives are provided in `SHA256SUMS.txt` with each release.

A release in the Microsoft Store is planned.

## Usage

VortexPanes runs in the background and is accessed through its icon in the notification area of the taskbar. The context menu provides the following entries:

- **Weather:** select the weather mode
- **Settings:** background image for rain droplets and information about the application
- **Exit**

Settings are stored in `%AppData%\VortexPanes`.

## Background Image for Rain Droplets

A water droplet on a window pane shows a smaller, inverted image of its surroundings. VortexPanes recreates this effect using a background image that can be selected under *Settings > Appearance*. Either one of the bundled images or a custom image can be used. Supported formats are JPEG, PNG, BMP, GIF and TIFF, as well as WebP and HEIC if the corresponding codecs are installed.

The screen content is not captured for this purpose. As a result, the animation is also visible in screenshots and screen recordings.

## Building from Source

Prerequisites:

- Visual Studio 2026 or the Build Tools for Visual Studio 2026 with the "Desktop development with C++" workload
- Individual components "MSVC Build Tools for x64/x86 (Latest)" and "MSVC Build Tools for ARM64/ARM64EC (Latest)"
- Windows SDK (10.0.26100 or later)
- CMake 4.2 or later; the version included with Visual Studio 2026 is sufficient (available in the Developer PowerShell for Visual Studio)
- Internet access for the first configuration: CMake downloads the Windows App SDK and C++/WinRT packages from nuget.org (pinned versions, verified by SHA256) into `out\nuget`, which all presets share

Build for x64:

```bat
cmake --preset x64
cmake --build --preset x64-release
```

The application is then located in `out\build\x64\bin\Release\`. For the other architectures, replace `x64` with `x86` or `arm64`. The complete portable folder, as contained in the release archives, is created with:

```bat
cmake --install out\build\x64 --config Release --prefix stage\x64
```

Running the application from the repository root:

```bat
out\build\x64\bin\Release\VortexPanes.exe
```

The application runs in the background; it is closed via *Exit* in the context menu of its notification area icon. Debug builds (`out\build\x64\bin\Debug\VortexPanes.exe`, built with `cmake --build --preset x64-debug`) additionally accept `--lang <tag>` (for example `--lang ja-JP`) to override the user interface language and `--warp` to force the WARP software renderer. They write the frame rate and frame time via `OutputDebugString`, which can be viewed with a debugger or DebugView.

Running the tests:

```bat
cmake --build --preset x64-debug
ctest --preset x64-debug
```

Continuous integration on GitHub Actions builds and tests all three architectures. ARM64 is tested natively on `windows-11-arm`.

## Creating a Release

1. Update the version in `CMakeLists.txt` (`project(VortexPanes VERSION x.y.z)`) and commit the change.
2. Create and push a tag:

   ```bat
   git tag vX.Y.Z
   git push origin vX.Y.Z
   ```

The release workflow creates an archive for each architecture and publishes it together with the checksums as a GitHub release.

## Project Structure

| Directory | Contents |
|---|---|
| `src/core` | Simulation of snow, rain and droplets; platform-independent and covered by unit tests |
| `src/platform` | Detection of windows, taskbar, monitors and full-screen applications (Win32) |
| `src/render` | Rendering with Direct3D 11 and DirectComposition, one overlay per monitor |
| `src/settings` | Settings window based on WinUI 3 (XAML Islands) |
| `src/app` | Application startup, main loop, notification area icon and configuration |
| `resources` | Icons, background images and language files |
| `cmake` | Integration of the Windows App SDK and C++/WinRT into the build |

Further information (in German): [Architecture](docs/ARCHITECTURE.md), [Roadmap](docs/ROADMAP.md)

## Contributing

Bug reports and suggestions are welcome via [Issues](https://github.com/lk-code/vortex-panes/issues). Translation improvements can be submitted directly as pull requests for the files in `resources/strings/`; `tools/Check-Strings.ps1` verifies that all language files are complete.

## License

Released under the [MIT License](LICENSE). Copyright (c) 2026 lk-code.

Notices for third-party components: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The release archives contain components of the Microsoft Windows App SDK, which are subject to the Microsoft Software License Terms.