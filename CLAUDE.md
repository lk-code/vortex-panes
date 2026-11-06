# CLAUDE.md – VortexPanes

- **Repository:** https://github.com/lk-code/vortex-panes (öffentlich, `main` ist der Standard-Branch)
- **Lizenz:** MIT, Copyright (c) 2026 lk-code
- **App-Name:** VortexPanes (Exe: `VortexPanes.exe`, Namespace `vp::`, CMake-Präfix `VP_`)

Windows-Desktop-Overlay mit **Wettermodi** (Leichter Schnee, Schneesturm, Sonne mit dezenten Sonnenstrahlen, Leichter Regen, Regensturm). Schneeflocken fallen über alle Monitore, sammeln sich auf den Oberkanten sichtbarer Fenster und der Taskleiste und fallen bzw. fliegen ab, wenn Fenster bewegt, in der Größe geändert, minimiert oder geschlossen werden. Mit dem Mauszeiger lässt sich der Schnee wegwischen. Bei Regen sammeln sich Wassertropfen auf den Kanten. Sie brechen ein wählbares Hintergrundbild wie echtes Wasser (mitgelieferte Bilder oder ein eigenes).

**Oberstes Ziel: maximale Performance.** Ergebnis ist pro Architektur eine kleine native `VortexPanes.exe`. Das Overlay hat keine Runtime-Abhängigkeiten, nur das Einstellungsfenster nutzt das Windows App SDK (lazy geladen).

**Öffentliches GitHub-Repo.** Es gehören keine Secrets, keine absoluten lokalen Pfade, keine persönlichen Daten und keine Build-Ausgaben (`out/`) ins Repo.

## Zielplattformen
| Architektur | CMake-Plattform | CI-Runner | Tests in CI |
|---|---|---|---|
| x86 (32 bit) | `Win32` | `windows-latest` | ja (läuft nativ auf x64) |
| x64 | `x64` | `windows-latest` | ja |
| ARM64 | `ARM64` | `windows-11-arm` | ja (nativ) |

Mindestversion: **Windows 11 21H2 (Build 22000)**, Entscheidung in M2. Ziel ist, dass die App auf einem frisch eingerichteten Windows 11 ohne Updates und ohne Nachinstallation läuft. Alle drei Architekturen sind gleichwertig: Jeder Commit muss für alle drei bauen und die Tests bestehen.

## Stack (fest)
- **C++20**, MSVC, **CMake ≥ 3.25** (`cmake_minimum_required`). Die Presets verlangen wegen des VS-2026-Generators CMake ≥ 4.2.
- Generator: **Visual Studio** (Multi-Config). Damit braucht es weder eine Entwickler-Eingabeaufforderung noch vcvars, und alle drei Architekturen laufen über `-A`. Der Generator-Name steht **nur an einer Stelle** im versteckten Basis-Preset: **`Visual Studio 18 2026`** (in M1 geprüft: VS 2026 18.11 lokal, auf `windows-latest` und auf `windows-11-arm`).
- Reines **Win32** für Overlay, Tray und Message-Loop. Kein MFC, kein Qt.
- **Einstellungsfenster: echtes WinUI 3** über **XAML Islands** (`DesktopWindowXamlSource` aus dem Windows App SDK) in einem Win32-Fenster, mit **C++/WinRT**. WinUI wird nur für dieses Fenster genutzt und erst beim ersten Öffnen geladen. Das Overlay bleibt reines D3D11 + DComp. Eingebunden über `cmake/WinAppSdk.cmake` (NuGet-Pakete mit fester Version und SHA256, kein MSBuild-Projekt). Details im Skill `winui-settings`.
- **Direct3D 11 + DXGI Flip-Model + DirectComposition** fürs Rendern
- HLSL-Shader werden **zur Build-Zeit** mit `fxc` aus dem Windows SDK kompiliert und als Header mit Bytecode erzeugt (siehe Skill `overlay-rendering`)
- COM über `Microsoft::WRL::ComPtr`
- Statische CRT über `CMAKE_MSVC_RUNTIME_LIBRARY = MultiThreaded$<$<CONFIG:Debug>:Debug>`. GoogleTest mit `gtest_force_shared_crt OFF`.
- Release: `/O2 /GL`, Linker `/LTCG /OPT:REF /OPT:ICF`
- Unicode (`UNICODE`, `_UNICODE`), `WIN32_LEAN_AND_MEAN`, `NOMINMAX`
- Tests: **GoogleTest** über CMake `FetchContent` mit fester Version (Tag oder Commit-Hash)
- Settings als **INI** über `GetPrivateProfileStringW`/`WritePrivateProfileStringW`
- **Freigegebene Abhängigkeiten:** Windows SDK, GoogleTest (BSD-3-Clause, nur für Tests, wird nicht ausgeliefert), **Windows App SDK** (`Microsoft.WindowsAppSDK` samt den von ihm benötigten Paketen) und **C++/WinRT** (`Microsoft.Windows.CppWinRT`), in CI nur offizielle `actions/*`-Actions. Die Lizenzen von Windows App SDK und C++/WinRT sind in M2 geprüft und in `THIRD_PARTY_NOTICES.md` dokumentiert. Verwendet werden nur die WinUI-Komponenten des Windows App SDK (nicht das Metapaket). Ihre Binärdateien stehen unter den Microsoft Software License Terms und liegen mit Lizenz- und Notice-Dateien im Ordner `licenses/` der ZIP. Für alles andere vorher fragen.

## Lizenzen von Fremdabhängigkeiten (Pflicht)
Das Projekt steht unter MIT und wird öffentlich auf GitHub sowie im Microsoft Store vertrieben. Bevor du **irgendeine** Fremdabhängigkeit vorschlägst (Bibliothek, Header-only-Code, kopierte Snippets, Fonts, Grafiken, Sounds, Actions von Drittanbietern), gilt:
1. Lizenz ermitteln, und zwar aus der LICENSE-Datei im Repo der Abhängigkeit, nicht nur aus einer Paketseite.
2. Nur **permissive** Lizenzen sind ohne Weiteres in Ordnung: MIT, BSD-2/3-Clause, Apache-2.0, zlib, Boost (BSL-1.0), ISC, Unlicense/CC0.
3. **Nicht erlaubt:** GPL, LGPL (statisch gelinkt), AGPL, MPL bei Code-Übernahme, SSPL, „Non-Commercial“-Lizenzen (CC-BY-NC usw.), Code ohne Lizenzangabe und alles, was den Store-Vertrieb oder eine kommerzielle Nutzung einschränkt. Im Zweifel: nicht verwenden, sondern fragen.
4. In der Rückfrage an mich nennst du Name, Version, Lizenz, Link und ob die Abhängigkeit ausgeliefert wird oder nur zum Build bzw. Test dient.
5. Nach meinem OK kommt ein Eintrag in **`THIRD_PARTY_NOTICES.md`** (Name, Version, Lizenz, Copyright, vollständiger Lizenztext). Ausgelieferte Abhängigkeiten erscheinen zusätzlich auf der Info-Seite der App und in der Release-ZIP.
6. Code von Stack Overflow, aus Blogs oder aus Microsoft-Samples nicht 1:1 kopieren, ohne die Lizenz zu prüfen. Microsoft-Samples stehen meist unter MIT und brauchen dann einen Hinweis.

## Portabilität x86 / x64 / ARM64 (Pflicht)
- Keine SSE-, AVX- oder NEON-Intrinsics und kein Inline-Assembler. Der Compiler darf autovektorisieren.
- Immer `GetWindowLongPtrW`/`SetWindowLongPtrW` und `GWLP_*` verwenden, nie die Nicht-Ptr-Varianten.
- HWND-IDs werden als `uint64_t` gespeichert (`reinterpret_cast<uintptr_t>`). Keine Annahmen über `sizeof(void*)`.
- Nur Typen mit fester Breite (`int32_t`, `uint64_t` …) in Datenstrukturen. `size_t` nur für Größen und Indizes.
- `CALLBACK`/`WINAPI` an allen Win32-Callbacks. Das ist auf x86 wichtig, weil dort `__stdcall` gilt.
- D3D-Device-Erstellung: zuerst Hardware, dann **WARP als Fallback** (für VMs und GPUs ohne passenden Treiber auf ARM).
- Keine Annahmen über Endianness und Alignment über `alignas` hinaus.

## Projektstruktur
```
CMakeLists.txt
CMakePresets.json
LICENSE  THIRD_PARTY_NOTICES.md  README.md  CLAUDE.md  .gitignore  .gitattributes  .editorconfig  .clang-format
.github/
  workflows/ci.yml        – Build und Test für alle drei Architekturen bei Push/PR
  workflows/release.yml   – bei Tag v*: Portable-ZIP pro Architektur + SHA256 an GitHub Release
  dependabot.yml          – Updates für GitHub Actions
cmake/         – WinAppSdk.cmake (Windows App SDK + C++/WinRT: Download, Projektion, Deployment), priconfig.xml, winappsdk_overrides.targets
src/
  core/        – vp_core (static lib): Simulation, plattformunabhängig, KEIN <windows.h>
  platform/    – vp_platform (static lib): Fenster-Tracking, Verdeckung, Monitore, Cursor, Vollbild
  render/      – vp_render (static lib): D3D11-Device, DComp, Swapchain pro Monitor, Pipelines
  settings/    – vp_settings (static lib): Einstellungsfenster, WinUI 3 über XAML Islands (C++/WinRT), lazy geladen
  app/         – VortexPanes.exe: wWinMain, Loop, Tray-Icon, Settings-Datei
shaders/       – *.hlsl
resources/     – app.manifest (PerMonitorV2), app.rc (Icons, VERSIONINFO, Dialoge), strings/<sprache>.rc (je eine STRINGTABLE),
                 backgrounds/*.jpg (Bilder für die Tropfen-Brechung, per Glob eingebettet),
                 app.ico (App), tray_light.ico / tray_dark.ico (Infobereich), icons/app_1024.png (Quelle)
tools/         – make_icons.py (erzeugt alle Icons aus Code, Pillow), make_demo_background.py (Demo-Bild), Check-Strings.ps1 (prüft Vollständigkeit der Übersetzungen)
packaging/     – zip/README.txt (Beilage der Portable-ZIP), AppxManifest.xml.in + Assets/ (MSIX, ab M10)
tests/         – core_tests, platform_tests (nur reine Logik)
docs/          – ARCHITECTURE.md, ROADMAP.md
```
Abhängigkeiten: `app → settings, render, platform, core`, `settings → core` (nur für App-Infos/Settings-Modell), `render → core`, `platform → core`. `core` kennt niemanden.
Alle Exes landen in `${CMAKE_BINARY_DIR}/bin/<Config>/` (`CMAKE_RUNTIME_OUTPUT_DIRECTORY`).

## CMake-Presets (Namen sind fest, CI nutzt sie)
- Configure: `x86`, `x64`, `arm64`, abgeleitet von einem versteckten `base` (Generator, `binaryDir = out/build/${presetName}`)
- Build: `x86-debug`, `x86-release`, `x64-debug`, `x64-release`, `arm64-debug`, `arm64-release`
- Test: `x86-debug`, `x64-debug`, `arm64-debug` (mit `output.outputOnFailure = true`)

## Befehle
- Konfigurieren: `cmake --preset x64`
- Bauen: `cmake --build --preset x64-debug`
- Tests: `ctest --preset x64-debug`
- Alle Architekturen bauen: `cmake --preset x86 && cmake --build --preset x86-release`, dasselbe für `x64` und `arm64`
- Start: `out\build\x64\bin\Debug\VortexPanes.exe`
- ARM64 lässt sich lokal auf einem x64-Rechner nur cross-kompilieren, nicht testen. Getestet wird in CI auf `windows-11-arm`.
- Voraussetzung lokal: Visual Studio 2026 (Build Tools) mit „Desktopentwicklung mit C++“, Windows SDK, **MSVC-Build-Tools für x86/x64 und ARM64** (Komponente `Microsoft.VisualStudio.Component.VC.Tools.ARM64`)
- `cmake`/`ctest` stammen aus der VS-Installation (`Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`) und sind in der „Developer PowerShell for VS 2026“ im PATH. `clang-format` liegt unter `VC\Tools\Llvm\x64\bin`.

## App-Verhalten (Shell)
- Die App hat **kein Taskleisten-Fenster**. Beim Start erscheint nur ein **Icon im Infobereich** (Tray).
- Kontextmenü des Icons (Rechtsklick): **Einstellungen**, Trennlinie, **Beenden**. Ein Doppelklick öffnet ebenfalls die Einstellungen.
- Das Einstellungsfenster (WinUI 3) ist vorerst eine **Info-Seite** mit App-Name und Icon, Entwickler, klickbarem Link zur Webseite und Version. Es wird später um echte Einstellungen erweitert, deshalb von Anfang an erweiterbar aufbauen (siehe Skill `winui-settings`).
- App-Metadaten als CMake-Cache-Variablen, die über `configure_file` in einen generierten Header und in `VERSIONINFO` fließen:
  - `VP_APP_NAME` = `VortexPanes`
  - `VP_DEVELOPER` = `lk-code` (überall als Entwickler- und Copyright-Name verwenden, nie einen Personennamen)
  - `VP_WEBSITE` = `https://www.lk-code.dev/`
  - `VP_REPOSITORY` = `https://github.com/lk-code/vortex-panes` (für die Lizenzhinweise und die README.txt in der ZIP)
  - Version aus `PROJECT_VERSION`
- **Mehrsprachig von Anfang an** (7 Sprachen): Englisch `en-US` (Fallback), Deutsch `de-DE`, Spanisch `es-ES`, Französisch `fr-FR`, Chinesisch vereinfacht `zh-CN`, Japanisch `ja-JP`, Koreanisch `ko-KR`. Windows wählt automatisch anhand der Anzeigesprache. Keine hartkodierten UI-Texte im Code, das gilt auch für Tray-Tooltip, Menü und Fehlermeldungen. Details im Skill `app-shell`.
- Icons werden nicht von Hand bearbeitet. Bei Änderungen `tools/make_icons.py` anpassen und neu ausführen, die erzeugten Dateien werden mit committet.

## Versionierung
- SemVer, Quelle der Wahrheit ist `project(VortexPanes VERSION x.y.z)` in `CMakeLists.txt`. Die Version fließt in `VERSIONINFO` (app.rc via `configure_file`).
- Releases per Git-Tag `vX.Y.Z`. Der Release-Workflow bricht ab, wenn Tag und CMake-Version nicht übereinstimmen.

## Konventionen
- Code, Bezeichner, Kommentare, Commit-Messages, Issues, Release Notes und **README.md** auf **Englisch**. `docs/` auf **Deutsch**.
- Namespace `vp::`, Dateien `snake_case.cpp/.h`, Typen `PascalCase`, Funktionen/Variablen `camelCase`, Member `m_`
- RAII überall (`UniqueHandle`, `UniqueHwnd` …, COM in `ComPtr`). Kein manuelles `Release()`.
- Keine Exceptions im Hot Path. `HRESULT` prüfen, `THROW_IF_FAILED` nur in der Initialisierung.
- **Null Heap-Allokationen pro Frame:** Puffer werden beim Start reserviert und danach nur geleert. GPU-Puffer laufen über `Map(WRITE_DISCARD)`.
- Simulation in **physischen Pixeln**, fester Takt `dt = 1/60 s` (Accumulator)
- Eigener deterministischer RNG (PCG32). Keine `std::*_distribution`.
- Win32-Aufrufe liegen hinter schmalen Schnittstellen, Logik als reine Funktionen und testbar.
- `/W4 /WX /permissive- /utf-8`, keine Warnungen auf **allen drei** Architekturen
- **Schreibstil:** Alle Texte im Repository (README, `docs/`, Release Notes, Commit-Messages, Code-Kommentare, UI-Texte, Store-Beschreibung) sind sachlich und professionell formuliert. **Keine Emojis**, keine Ausrufezeichen-Häufungen, keine Werbesprache, keine dekorativen Sonderzeichen. Fettdruck nur sparsam für UI-Begriffe. Die README ist neutral und unpersönlich formuliert (kein „you“-Marketington).
- **Git-Workflow:** Es wird direkt auf **`main`** gearbeitet. Keine Feature-Branches, keine Pull Requests. Committe in kleinen, sinnvollen Schritten, also nach jedem abgeschlossenen Teilschritt mit grünem Build und grünen Tests, nicht erst am Ende eines Meilensteins. `git push` nur nach meinem OK.
- Commits: Conventional Commits (`feat:`, `fix:`, `ci:`, `docs:` …)

## Performance-Budget
- Schneefall „mittel“, 2× 1440p @ 60 FPS: **< 1 % CPU**, **< 40 MB RAM**
- Regensturm mit Lichtbrechung, 2× 1440p @ 60 FPS: **< 2 % CPU**, GPU moderat. Die Brechung nutzt ein Hintergrundbild als GPU-Textur, es gibt **keine Bildschirmaufnahme**.
- Modus Sonne nach dem Abtauen: nur noch Sonnenstrahlen mit 15 FPS, **< 0,3 % CPU**
- Pausiert: **0 % CPU**, die Loop wartet nur auf Nachrichten
- Release-Exe: < 1 MB pro Architektur. Die Portable-ZIP ist durch die WinAppSDK-Laufzeit größer (self-contained), das ist akzeptiert. Das MSIX bleibt klein (framework-abhängig).
- Einstellungsfenster nie geöffnet: Es wird keine WinUI-DLL geladen (keine statischen Importe, Auto-Initializer aus, Initialisierung erst beim ersten Öffnen; prüfen mit Process Explorer, `listdlls` oder `tasklist /m`). Fenster offen: < 150 MB RAM sind ok.

## Nicht-Ziele (v1)
- Kein macOS/Linux, kein ARM64EC
- Kein Schnee auf UI-Elementen innerhalb von Fenstern
- Kein WinUI im Overlay oder Tray, nur im Einstellungsfenster
- Kein Installer und keine Code-Signierung der Exes (README erklärt die SmartScreen-Warnung). Den Store-Vertrieb gibt es als MSIX, siehe M10.

## Arbeitsweise pro Meilenstein
Gearbeitet wird anhand von `docs/ROADMAP.md`. Jeder Meilenstein wird mit einem eigenen Prompt gestartet und umfasst genau diesen Meilenstein.
1. Zu Beginn `CLAUDE.md`, `docs/ARCHITECTURE.md`, den Abschnitt des Meilensteins in `docs/ROADMAP.md` und die dort genannten Skills lesen. Danach eine Task-Liste anlegen.
2. Weicht etwas von den Dokumenten ab oder ist etwas unklar, vor der Umsetzung fragen. Abweichungen nach Freigabe in `docs/ARCHITECTURE.md` unter „Entscheidungen“ festhalten.
3. Direkt auf `main` arbeiten und in kleinen Schritten committen (Conventional Commits). `git push` nur nach Freigabe.
4. Vor Abschluss: Debug- und Release-Build für x86, x64 und ARM64 ohne Warnungen, `ctest` grün (x86 und x64 lokal, ARM64 über die CI), alle Texte in 7 Sprachen, README und Doku aktuell.
5. Abschluss mit einer kurzen Zusammenfassung: was umgesetzt wurde, Abweichungen, eine Checkliste für den manuellen Test. Danach stoppen und auf Freigabe warten. Den nächsten Meilenstein nicht selbstständig beginnen.

## Skills
- `.claude/skills/win32-window-tracking`: Fensterliste, Z-Order, Verdeckung, Taskleiste, DPI, Hooks, Vollbild
- `.claude/skills/snow-simulation`: Flocken, Höhenprofile, Abrutschen, Abwerfen, Cursor-Interaktion
- `.claude/skills/weather-and-rain`: Wettermodi, Übergänge, Sonne, Regen, Tropfen, Lichtbrechung über Hintergrundbild, Bildauswahl
- `.claude/skills/overlay-rendering`: Click-Through-Fenster, D3D11 + DirectComposition, Shader, Frame-Pacing
- `.claude/skills/build-and-release`: CMake-Presets, Multi-Arch, fxc-Suche, GitHub Actions, Releases, MSIX/Store
- `.claude/skills/app-shell`: Tray-Icon, Kontextmenü, Single-Instance, Ressourcen, Lokalisierung
- `.claude/skills/winui-settings`: Einstellungsfenster mit WinUI 3 über XAML Islands, Build-Einbindung, Deployment