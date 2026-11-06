---
name: build-and-release
description: Use when touching CMakeLists.txt, CMakePresets.json, shader compilation, compiler flags, the x86/x64/ARM64 build matrix, .github/workflows (CI, release), versioning or anything about publishing the public GitHub repo.
---

# Build & Release (x86 / x64 / ARM64)

## CMakePresets.json (Skelett)
```json
{
  "version": 6,
  "configurePresets": [
    { "name": "base", "hidden": true,
      "generator": "Visual Studio 17 2022",
      "binaryDir": "${sourceDir}/out/build/${presetName}" },
    { "name": "x86",   "inherits": "base", "architecture": { "value": "Win32", "strategy": "set" } },
    { "name": "x64",   "inherits": "base", "architecture": { "value": "x64",   "strategy": "set" } },
    { "name": "arm64", "inherits": "base", "architecture": { "value": "ARM64", "strategy": "set" } }
  ],
  "buildPresets": [
    { "name": "x64-debug",   "configurePreset": "x64", "configuration": "Debug" },
    { "name": "x64-release", "configurePreset": "x64", "configuration": "Release" }
    // … analog für x86 und arm64
  ],
  "testPresets": [
    { "name": "x64-debug", "configurePreset": "x64", "configuration": "Debug",
      "output": { "outputOnFailure": true } }
    // … analog für x86 und arm64
  ]
}
```
- In M1 prüfen: Welche VS-Version ist lokal installiert und welche auf `windows-latest` bzw. `windows-11-arm`? Die Liste der installierten Software steht in `actions/runner-images` unter `images/windows/…-Readme.md`. Danach den Generator-Namen im `base`-Preset entsprechend setzen. Er steht nur dort.

## CMakeLists.txt: Pflichtpunkte
- Hintergrundbilder: `file(GLOB VP_BACKGROUNDS CONFIGURE_DEPENDS resources/backgrounds/*.jpg)` → generierte `.rc`-Einträge (`RCDATA`) und ein generierter Header mit der Bildtabelle. Neue Bilder brauchen nur ein erneutes Konfigurieren, keine Codeänderung.
- `install()`-Regeln für die Exe **und** alle Laufzeitdateien (WinAppSDK-DLLs, `.pri`, ggf. `.winmd`), damit `cmake --install` einen lauffähigen Ordner erzeugt. CI und Release nutzen ausschließlich diesen Weg, nicht `bin/Release` direkt.
- `cmake_minimum_required(VERSION 3.25)`, `project(VortexPanes VERSION 0.1.0 LANGUAGES CXX)`
- `set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")` **vor** dem Anlegen aller Targets und vor `FetchContent_MakeAvailable(googletest)`
- `set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)`, `set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)`
- `set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)`. Beim VS-Generator wird daraus automatisch `bin/<Config>/`.
- Optionen als Interface-Target `vp_options` (Warnungen, Defines, `/utf-8`, `/permissive-`, `/W4 /WX`), das alle eigenen Targets linken. **Nicht** global, sonst trifft `/WX` auch GoogleTest.
- `$<$<CONFIG:Release>:/GL>` sowie Linker `$<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>`
- `VortexPanes` als `WIN32`-Executable (Subsystem Windows, `wWinMain`). Linken: `d3d11 dxgi dcomp dwmapi shcore user32 gdi32 shell32 ole32 wtsapi32 advapi32`
- Manifest: `resources/app.manifest` als Source hinzufügen. MSVC bettet es ein. Inhalt: PerMonitorV2 (`<dpiAwareness>PerMonitorV2</dpiAwareness>`), `supportedOS` für Windows 10/11, Common Controls 6.
- `app.rc.in` → `configure_file` → `app.rc` mit `VERSIONINFO` aus `PROJECT_VERSION`
- `enable_testing()` + `include(GoogleTest)` + `gtest_discover_tests(... DISCOVERY_MODE PRE_TEST)`

## fxc finden (alle Hosts inkl. ARM64)
```cmake
# Windows SDK root + version provided by the VS generator
set(_sdk_bin "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
find_program(FXC_EXECUTABLE fxc
  HINTS "${_sdk_bin}/${CMAKE_HOST_SYSTEM_PROCESSOR}" "${_sdk_bin}/arm64" "${_sdk_bin}/x64" "${_sdk_bin}/x86"
  REQUIRED NO_DEFAULT_PATH)
```
- `CMAKE_HOST_SYSTEM_PROCESSOR` liefert `AMD64` oder `ARM64`. Die Ordner heißen `x64` bzw. `arm64`, deshalb mappen oder alle Hints probieren, wie oben gezeigt. Auf ARM64-Hosts läuft notfalls die x64-`fxc` per Emulation.
- Shader per `add_custom_command(OUTPUT …/flake_vs.h COMMAND ${FXC_EXECUTABLE} /nologo /O3 /T vs_4_0 /E main /Fh … /Vn g_flakeVS … DEPENDS …)`. Die Header landen in `${CMAKE_CURRENT_BINARY_DIR}/shaders` und gehören zu den Include-Dirs von `vp_render`.
- Der Shader-Bytecode (DXBC) ist architekturunabhängig. Der Treiber kompiliert ihn auf ARM64 genauso.

## CI (`.github/workflows/ci.yml`)
- Läuft bei Push auf `main` und bei jedem PR. Matrix: `x86` und `x64` auf `windows-latest`, `arm64` auf `windows-11-arm`. ARM-Runner sind für öffentliche Repos kostenlos.
- Schritte: Checkout → `cmake --preset <arch>` → Build Debug → `ctest` → Build Release → Upload der Release-Exe als Artifact
- `permissions: contents: read`, `concurrency` bricht alte Läufe desselben Branches ab
- Actions in der Hauptversion pinnen (`actions/checkout@v4` usw.), Dependabot hält sie aktuell
- Ein FetchContent-Cache ist optional. GoogleTest ist klein, Caching erst bei Bedarf einführen.

## Release (`.github/workflows/release.yml`)
- Trigger: Push eines Tags `v*.*.*`
- Job 1 prüft, ob der Tag zur `project(VERSION)` passt. Sonst schlägt er fehl.
- Job 2 baut in der Matrix wie CI und testet. Danach erzeugt `cmake --install out/build/<arch> --config Release --prefix stage/app` den vollständigen App-Ordner (Exe plus WinAppSDK-Laufzeit für Self-Contained, ohne `.pdb`, `.lib` und `.exp`). Daraus wird pro Architektur eine **unsignierte Portable-ZIP** `VortexPanes-<version>-<arch>.zip` mit dem App-Ordner, `LICENSE.txt`, `THIRD_PARTY_NOTICES.txt` und `README.txt` (aus `packaging/zip/README.txt`, Englisch, mit Start, SmartScreen-Hinweis, Deinstallation und Architekturwahl).
- Job 3 lädt alle Artifacts herunter, erzeugt `SHA256SUMS.txt` und ruft `gh release create <tag> --generate-notes --verify-tag <files>` auf. Dafür braucht der Job `permissions: contents: write`, der Token ist `GITHUB_TOKEN`. Keine Drittanbieter-Actions.
- Die ZIPs gibt es **zusätzlich** zum Store-Paket aus M10, nicht als Ersatz. Das Store-Bundle hängt als eigene Datei am selben Release.
- Keine Code-Signierung (Nicht-Ziel v1)

## Öffentliches Repo: Checkliste
- `LICENSE` (MIT, „Copyright (c) 2026 lk-code“)
- `THIRD_PARTY_NOTICES.md`: wird bei jeder neuen Fremdabhängigkeit gepflegt (Regeln in `CLAUDE.md`)
- **`.gitignore`** (von Claude Code in M1 anlegen, kommentiert und nach Bereichen gruppiert). Muss mindestens abdecken:
  - Build-Ausgaben: `out/`, `build/`, `stage/`, `dist/`, `[Bb]in/`, `[Oo]bj/`, `x64/`, `x86/`, `ARM64/`, `Win32/`, `Debug/`, `Release/`, `RelWithDebInfo/`, `MinSizRel/`
  - CMake: `CMakeCache.txt`, `CMakeFiles/`, `cmake_install.cmake`, `CTestTestfile.cmake`, `_deps/`, `CMakeUserPresets.json`, `compile_commands.json`
  - Visual Studio / MSVC: `.vs/`, `*.user`, `*.suo`, `*.VC.db`, `*.VC.opendb`, `*.ipch`, `ipch/`, `*.pdb`, `*.ilk`, `*.obj`, `*.lib`, `*.exp`, `*.idb`, `*.iobj`, `*.ipdb`, `*.tlog`, `*.log`, `*.aps`, `*.res`
  - NuGet / Windows App SDK: `packages/`, `*.nupkg` (heruntergeladene Pakete), generierte C++/WinRT-Projektionen (`Generated Files/`), `*.pri` außerhalb von `resources/`
  - MSIX: `*.msix`, `*.msixbundle`, `*.appx`, `*.appxbundle`, `*.msixupload`, `AppPackages/`, **`*.pfx`** (Testzertifikate dürfen niemals ins Repo)
  - Generierte Shader-Header (liegen im Build-Ordner, zur Sicherheit `shaders/*.h` ausschließen, falls versehentlich im Quellbaum erzeugt)
  - Python: `__pycache__/`, `*.pyc`, `.venv/`
  - Editoren und OS: `.vscode/` (außer geteilten Einstellungen per `!.vscode/extensions.json`), `.idea/`, `*.swp`, `.DS_Store`, `Thumbs.db`, `desktop.ini`
  - Claude Code: `.claude/settings.local.json` (persönlich), **nicht** `.claude/settings.json` und **nicht** `.claude/skills/`, die gehören ins Repo
  - Lokale Secrets: `.env`, `*.local.*`
  - **Nicht** ignorieren: `resources/**/*.ico`, `resources/icons/*.png`, `packaging/**`, `.github/**`
  - Nach dem Anlegen mit `git status --ignored` prüfen, dass kein Quell- oder Ressourcendatei-Typ versehentlich ignoriert wird.
- `.gitattributes`: `* text=auto`, `*.sln eol=crlf`, `*.ico binary`, `*.png binary`, `*.gif binary`
- README mit Badges (CI-Status, neuestes Release, Lizenz), Download-Tabelle pro Architektur und Hinweis, welche Exe man nimmt (x64 = Standard-PC, ARM64 = Snapdragon/Surface, x86 = alte 32-bit-Systeme), sowie einem Hinweis auf die SmartScreen-Warnung
- `.github/dependabot.yml` für `github-actions` (wöchentlich)
- Keine Secrets, keine lokalen Pfade, keine Debug-Dumps, keine großen Binärdateien im Repo. Das README-GIF darf höchstens etwa 5 MB groß sein.

## Microsoft Store (MSIX), Meilenstein M10
- Ziel: zusätzlich zu den Exes auf GitHub ein **`.msixbundle`** (x86 + x64 + ARM64) für den Store. Native Win32-Apps sind dort als **Full-Trust-Desktop-App** erlaubt (Capability `runFullTrust`). WinUI ist nicht nötig.
- Paket bauen ausschließlich mit Windows-SDK-Tools: `makeappx pack` pro Architektur, `makeappx bundle` → `.msixbundle`, `signtool` nur für lokale Tests mit einem selbstsignierten Zertifikat. **Für den Store nicht signieren**, das macht Microsoft beim Einreichen.
- `packaging/AppxManifest.xml.in` → `configure_file`. Werte:
  - `Identity Name`, `Publisher`, `PublisherDisplayName` aus Partner Center. Sie sind nicht geheim und dürfen ins öffentliche Repo, als CMake-Cache-Variablen mit Platzhaltern, bis die echten Werte da sind.
  - `Version` = `PROJECT_VERSION` + `.0` (MSIX braucht 4 Stellen, die letzte muss für den Store 0 sein)
  - `ProcessorArchitecture` = x86 / x64 / arm64
  - `TargetDeviceFamily Name="Windows.Desktop" MinVersion="10.0.19044.0"`
  - `<rescap:Capability Name="runFullTrust"/>`
  - `Application EntryPoint="Windows.FullTrustApplication" Executable="VortexPanes.exe"`
  - Logos (`packaging/Assets/`: Square44x44, Square150x150, Wide310x150, StoreLogo, inkl. `targetsize`-Varianten). Sie werden von `tools/make_icons.py` erzeugt, das Skript dafür erweitern.
- **Unterschiede im Paket**, über `GetCurrentPackageFullName()` erkennen (`APPMODEL_ERROR_NO_PACKAGE` → unpaketiert):
  - Autostart: Im Paket `desktop:Extension Category="windows.startupTask"` (`StartupTask`-ID im Manifest) und das Ein-/Ausschalten über die WinRT-API `Windows.ApplicationModel.StartupTask` (C++/WinRT ist im Windows SDK enthalten, kein Extra-Paket). Unpaketiert der Registry-Schlüssel `HKCU\…\Run`.
  - Settings: `%AppData%` wird im Paket umgeleitet, das funktioniert ohne Änderung.
  - Keine Selbst-Updates. Updates kommen über den Store.
- CI: Ein Job `package-msix` im Release-Workflow lädt die drei Release-Exes (eigenes Artifact neben den ZIPs), packt sie, bündelt sie zu `VortexPanes-<version>.msixbundle` und hängt das Bundle unsigniert an das GitHub Release, damit es manuell in Partner Center hochgeladen werden kann. Die automatische Einreichung über die Store-Submission-API erfolgt erst später und nur nach Rückfrage, weil sie Secrets braucht.
- Store-Hinweise für das Listing: Datenschutzerklärung (URL, die App sammelt keine Daten), Screenshots, Altersfreigabe-Fragebogen. Die Begründung für `runFullTrust`: „Overlay rendering across all windows and window tracking require full Win32 access.“
