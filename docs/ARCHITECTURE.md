# Architektur

## Überblick

```
┌──────────────────── VortexPanes.exe (app) ─────────────────────┐
│ wWinMain                                                       │
│  └─ MainLoop: MsgWaitForMultipleObjectsEx(Frame-Timer, Msgs)   │
│       ├─ tick(dt) ──────────► vp::World + Weather   (core)     │
│       ├─ snapshot ◄────────── WindowTracker         (platform) │
│       └─ draw ──────────────► Renderer              (render)   │
│                                 └─ MonitorOverlay[] (1 pro Monitor:
│                                     HWND + DComp-Visual + Swapchain)
│  TrayIcon (Infobereich, kein Taskleisten-Fenster) · SettingsWindow (Info-Seite)
│  Settings (INI) · Single-Instance (Mutex)
└────────────────────────────────────────────────────────────────┘
```

Alles läuft auf **einem Thread** (UI-Thread). Es gibt keine Synchronisation und keine Races. Die Simulation einiger tausend Partikel kostet wenige Mikrosekunden.

## Datenfluss pro Frame
1. **Warten:** `MsgWaitForMultipleObjectsEx` auf einen High-Resolution-Waitable-Timer (Ziel-FPS) **oder** Fensternachrichten. Nachrichten werden abgearbeitet, darunter WinEvent-Hooks, Tray, Display-Change und Session-Lock.
2. **Snapshot:** Wenn ein Hook das Dirty-Flag gesetzt hat oder 100 ms vergangen sind, liefert `WindowTracker` eine neue `SurfaceList`. Sie enthält Fenster und Taskleisten mit HWND-ID, Rect (physische Pixel) und Z-Index, von oben nach unten sortiert.
3. **Verdeckung:** `computeVisibleTopSegments()` liefert pro Fläche die sichtbaren Abschnitte ihrer Oberkante.
4. **Simulation:** `World::update(dt, surfaces, cursor)` übernimmt mehrere Aufgaben:
   - Diff zum letzten Snapshot: bewegt, Fling, Größe geändert, verschwunden
   - Flocken integrieren und Kollision mit sichtbaren Segmenten
   - Schneedecken: ablagern, abrutschen, schmelzen
   - Cursor-Brush
5. **Rendern:** Pro Monitor werden die sichtbaren Flocken in einen Instanzpuffer und die Schneedecken in einen dynamischen Vertexpuffer geschrieben. Dann folgen zwei Draw-Calls und `Present`.
6. **Pause:** Bei Vollbild-App, Sperre oder Deaktivierung wird einmal transparent präsentiert. Danach ist der Timer aus, und die Loop wartet nur noch auf Nachrichten.

## Kernmodelle (core)
- `Flake`: POD mit `x, y, vx, vy, radius, phase, depth, alpha`
- `FlakePool`: `std::vector<Flake>` mit fester Kapazität und Swap-Remove
- `Pcg32`: deterministischer Zufallsgenerator (PCG-XSH-RR) mit Seed und Stream
- `FixedTimestep`: Accumulator für den festen Takt von 60 Hz (`kSimulationRate`, `kTickSeconds`), höchstens 4 Ticks pro Frame
- `WindField`: Wind als zustandslose Value-Noise über die Simulationszeit
- `Snowfall` mit `SnowfallParams`: fallende Flocken über alle Monitore (Spawn, Integration, Wrap, Entfernen, Vorlauf). `SnowColumn` beschreibt einen senkrechten Streifen des virtuellen Bildschirms mit oberstem und unterstem Monitorrand. Ab M4/M5 wird `Snowfall` Teil von `World`.
- `FlakeInstance` und `collectVisibleFlakes()`: GPU-Instanzdaten und Auswahl der Flocken pro Monitor
- `SnowCap`: pro Fläche (Schlüssel: HWND als `uint64_t`). Enthält `std::vector<float> heights` (eine Spalte pro `columnWidth` px, relativ zur linken Kante), letztes Rect und geglättete Geschwindigkeit. Caps liegen in einem `std::vector` mit Index-Map, damit Iterieren cachefreundlich ist.
- `Surface { uint64_t id; RectI rect; int z; SurfaceKind kind; }` mit `kind` = Window | Taskbar | Ground
- `Settings`: Intensität, Wind, maximale Höhe, Schmelzrate, FPS, Flockengröße, Autostart, Debug. Persistiert in `%AppData%\VortexPanes\settings.ini`.

## Verhaltensregeln
| Ereignis | Verhalten |
|---|---|
| Fenster langsam verschoben (Geschwindigkeit < `flingThreshold`) | Schnee bleibt liegen und wandert mit |
| Fenster schnell verschoben („Fling“) | Decke wird zu Partikeln mit der Fenstergeschwindigkeit plus Streuung |
| Breite oder Höhe ändert sich | Decke fällt als Partikel herunter |
| Fenster minimiert, geschlossen, cloaked | Decke fällt als Partikel herunter |
| Fenster teilweise verdeckt | Schnee bleibt erhalten, nur der sichtbare Teil wird gezeichnet und befüllt |
| Cursor fährt durch die Schneedecke | Spalten im Radius werden abgetragen, Partikel fliegen in Cursor-Richtung |
| Vollbild-App im Vordergrund oder Bildschirm gesperrt | Pausieren, 0 % CPU |
| Maximiertes Fenster (Oberkante = Monitorrand) | Kein Schnee |

## App-Shell (Details im Skill `app-shell`)
- Ein **verstecktes Top-Level-Fenster** (`WS_POPUP`, `WS_EX_TOOLWINDOW`, wird nie angezeigt) empfängt Tray-Nachrichten, `TaskbarCreated`, `WM_SETTINGCHANGE` und Hotkeys. Ein Message-Only-Fenster scheidet aus, weil es keine Broadcasts erhält (siehe „Entscheidungen“).
- `TrayIcon`: `Shell_NotifyIconW` mit Version 4. Kontextmenü **Einstellungen** und **Beenden**. Ist die Taskleiste beim Start noch nicht bereit, wird das Symbol 30 s lang jede Sekunde erneut angelegt, danach erscheint eine Fehlermeldung.
- `SettingsWindow` (Modul `src/settings`): Win32-Fenster mit **WinUI 3 über XAML Islands** (Windows App SDK, C++/WinRT). Es wird bei Bedarf erzeugt und beim Schließen zerstört, höchstens eine Instanz. Die WinUI-Laufzeit wird erst beim ersten Öffnen geladen. Inhalt vorerst die Info-Seite. Es ist das einzige Fenster der App mit Taskleisten-Eintrag, und auch das nur, solange es offen ist.
  - `XamlRuntime` initialisiert beim ersten Öffnen einmal pro Thread: STA, `DispatcherQueueController`, eine eigene `Application`-Unterklasse (Metadaten der Standard-Controls über `XamlControlsXamlMetaDataProvider`, Fluent-Ressourcen über `XamlControlsResources`) und `WindowsXamlManager`. Beim Beenden der App wird alles in umgekehrter Reihenfolge freigegeben.
  - Die Seite kommt als XAML ohne Texte aus einer RCDATA-Ressource (`XamlReader::Load`). Texte, App-Infos und eingebettete Daten liefert die App als `SettingsContent`, die Abhängigkeit `settings → core` bleibt dadurch bestehen.
  - Die Main-Loop gibt jede Nachricht zuerst an `ContentPreTranslateMessage` (über `SettingsWindow::preTranslateMessage`), damit Tab, Pfeiltasten und Zugriffstasten funktionieren.
- **Deployment des Windows App SDK** (`cmake/WinAppSdk.cmake`): self-contained. Die Laufzeitdateien liegen neben der Exe, die Klassen werden über RegFree-WinRT-Einträge im Anwendungsmanifest aktiviert, und eine gemergte `VortexPanes.pri` liefert die WinUI-Themenressourcen. Die Exe importiert keine DLL des Windows App SDK statisch.

## Rendering (Kurzfassung, Details im Skill)
- Pro Monitor ein `WS_POPUP`-Fenster mit `WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE`
- Ein gemeinsames D3D11-Device. Pro Monitor eine `CreateSwapChainForComposition`-Swapchain (BGRA8, `DXGI_ALPHA_MODE_PREMULTIPLIED`, Flip-Sequential) an einem DComp-Visual
- Flocken: **ein instanced Draw-Call** (Quad mit 4 Vertices × N Instanzen). Der Pixel-Shader zeichnet einen weichen Kreis.
- Schneedecken: **ein Draw-Call** mit Triangle-List aus allen sichtbaren Segmenten

## Entscheidungen
- 2026-11-06: Natives C++ mit D3D11 und DirectComposition statt .NET/WPF. Grund: maximale Performance, keine Readbacks von Layered Windows, eine einzelne kleine Exe ohne Runtime.
- 2026-11-06: Einstellungsfenster mit echtem WinUI 3 (XAML Islands) statt Win32-Dialog oder nachgebauter Fluent-Optik. Grund: originales Windows-11-Design, Barrierefreiheit inklusive. Folgen: Abhängigkeit vom Windows App SDK, größere Portable-ZIP (self-contained), Lazy Loading, damit das Overlay unbeeinflusst bleibt.
- 2026-11-06: Oberfläche in 7 Sprachen (en, de, es, fr, zh-CN, ja, ko) aus einer gemeinsamen STRINGTABLE für Tray und Einstellungen.
- 2026-11-06: Wettermodi (Leichter Schnee, Schneesturm, Sonne, Leichter Regen, Regensturm). Regentropfen mit Lichtbrechung über ein wählbares Hintergrundbild (mitgeliefert oder eigenes). Bewusst **keine** Bildschirmaufnahme: Die Effekte bleiben auf Screenshots und Aufnahmen sichtbar, es gibt keine Datenschutzfragen, und es funktioniert per Remote Desktop.
- 2026-11-06: Git-Workflow: direkt auf `main`, kleine Commits, Push nur nach Freigabe.
- 2026-11-06 (M1): CMake-Generator **`Visual Studio 18 2026`**. Grund: Lokal und auf beiden CI-Runnern (`windows-latest` = `windows-2025-vs2026`, `windows-11-arm`) ist Visual Studio 2026 18.11 installiert. Folge: Die Presets verlangen CMake ≥ 4.2 (in VS 2026 enthalten), `CMakeLists.txt` bleibt bei `cmake_minimum_required(VERSION 3.25)`.
- 2026-11-06 (M1): Das Shell-Fenster ist ein **verstecktes Top-Level-Fenster** statt eines Message-Only-Fensters. Grund: Message-Only-Fenster erhalten keine Broadcasts, also weder `TaskbarCreated` (Explorer-Neustart) noch `WM_SETTINGCHANGE` (Taskleisten-Theme). Die zweite Instanz findet die laufende per `FindWindowW(L"VortexPanes.Shell", nullptr)`.
- 2026-11-06 (M1): `.gitignore` ignoriert nur `/ROADMAP.md` im Wurzelverzeichnis (private Prompt-Sammlung). `docs/ROADMAP.md` wird versioniert.
- 2026-11-06 (M1): Generische Hilfen ohne `<windows.h>` liegen in `core`: `UniqueResource<Traits>` (RAII für Handles, die Module definieren eigene Traits wie `UniqueHwnd`), `HResultError`/`THROW_IF_FAILED` und der generierte Header `core/app_info.h`. Grund: Die Abhängigkeiten `render → core` und `platform → core` bleiben unverändert, und `settings` erreicht die App-Infos über `core`.
- 2026-11-06 (M1): Der Frame-Timer wird pro Frame mit relativer Fälligkeit aus einer QPC-Deadline neu gesetzt (`core/frame_pacer`), nicht periodisch. Grund: Die Periode von `SetWaitableTimerEx` ist ganzzahlig in Millisekunden, 16 ms ergäben 62,5 FPS. Fällt die Schleife zurück, werden verpasste Frames verworfen statt nachgeholt.
- 2026-11-06 (M1): UI-Texte werden mit `FindResourceExW` gezielt in der gewählten Sprache geladen (Fallback en-US), statt sich auf die Sprachwahl von `LoadStringW` zu verlassen. `SetThreadPreferredUILanguages` wird trotzdem gesetzt (für System-UI wie MessageBox-Schaltflächen). Traditionelles Chinesisch (zh-TW, zh-HK, zh-MO, zh-Hant) fällt auf Englisch statt auf vereinfachtes Chinesisch.
- 2026-11-06 (M1): Das Einbetten der Hintergrundbilder (Glob, RCDATA) folgt in M8 und `THIRD_PARTY_NOTICES` als RCDATA in M2, jeweils dort, wo sie genutzt werden. So bleibt die Exe in M1 klein.
- 2026-11-06 (M2): **Mindestversion Windows 11 21H2 (Build 22000)** statt Windows 10 21H2. Grund: Vorgabe von Lars; die App soll auf einem frisch eingerichteten Windows 11 ohne Updates und ohne Nachinstallation laufen.
- 2026-11-06 (M2): Windows App SDK **2.6.5**, davon nur die **WinUI-Komponenten** (`Microsoft.WindowsAppSDK.WinUI`, `.InteractiveExperiences`, `.Foundation`, `.Base`) statt des Metapakets `Microsoft.WindowsAppSDK`, dazu **C++/WinRT 3.0.260818.1**. Grund: Das Metapaket brächte AI- und ML-Komponenten (ONNX Runtime) mit, etwa 40–50 MB mehr pro Architektur und zusätzliche Drittanbieter-Lizenzen, ohne Nutzen für das Einstellungsfenster. Das Runtime-Paket wird nur für die framework-abhängige Variante (MSIX, M10) geladen.
- 2026-11-06 (M2): Der Build bleibt **CMake**, kein MSBuild-Projekt. Die Pakete werden beim Konfigurieren per `file(DOWNLOAD)` mit fester Version und SHA256 nach `out/nuget` geladen, die C++/WinRT-Projektion erzeugt `cppwinrt.exe` beim Konfigurieren, und die MSBuild-Logik des Windows App SDK für das Self-Contained-Deployment wird per `VS_PROJECT_IMPORT` nur in das Exe-Projekt importiert. Grund: ein Build-System für alles (Overlay, Shader, Tests, Presets, CI). Der XAML-Compiler, der Hauptvorteil von MSBuild, wird nicht gebraucht, weil das XAML zur Laufzeit geladen wird.
- 2026-11-06 (M2): Eine gemergte **`VortexPanes.pri`** wird per `makepri` aus dem Windows SDK erzeugt (PRI-Indexer über die `.pri`-Dateien der Komponenten). Grund: Unpaketiert sucht WinUI seine Themenressourcen (`ms-appx:///Microsoft.UI.Xaml/Themes/themeresources.xaml`) im Ressourcenindex der Anwendung; ohne ihn scheitert `XamlControlsResources`. Das erledigt sonst das MSIX-Tooling.
- 2026-11-06 (M2): Alle **Auto-Initializer des Windows App SDK sind aus** (Bootstrap, UndockedRegFreeWinRT, DeploymentManager). Ab Windows 10 1903 aktiviert das Betriebssystem die Klassen über die RegFree-WinRT-Einträge im Manifest selbst. Folge: Beim Start lädt keine DLL des Windows App SDK, `/DELAYLOAD` ist nicht nötig. `ContentPreTranslateMessage` wird per `GetProcAddress` aufgelöst.
- 2026-11-06 (M2): XAML Islands brauchen eine **eigene `Application`-Unterklasse** (abweichend vom ursprünglichen Wortlaut des Skills „ohne WinUI-`Application`“). Das WinUI-App-Modell (`Application::Start`) wird trotzdem nicht genutzt.
- 2026-11-06 (M2): Weitere Erkenntnisse zu Islands: Mica (`DesktopWindowXamlSource::SystemBackdrop`) erst nach dem Setzen von `Content` zuweisen, sonst stürzt `Microsoft.UI.Xaml.dll` ab. Den Win32-Fokus auf das Island-Kindfenster legen und `NavigateFocus` erst nach `Loaded` aufrufen. `TakeFocusRequested` während eines eigenen `NavigateFocus`-Aufrufs ignorieren, sonst entsteht eine Endlosrekursion.
- 2026-11-06 (M2): **Lizenzen in der Portable-ZIP:** Die Laufzeitdateien des Windows App SDK stehen unter den Microsoft Software License Terms. Lizenztext und Notice-Dateien liegen in `licenses/`, `README.txt` weist darauf hin, und `THIRD_PARTY_NOTICES.md` (auch im Lizenzdialog der App) enthält den vollen Text. `.winmd`-Metadaten und `Microsoft.WindowsAppRuntime.Bootstrap.dll` werden nicht ausgeliefert, C++ braucht sie zur Laufzeit nicht. Offen für M10: Die Lizenz erlaubt dem SDK Datenerhebung (§2), das ist bei der Datenschutzangabe für den Store zu berücksichtigen.
- 2026-11-06 (M2): Das Modul `settings` hat ein eigenes Debug-Log (`settings/settings_log.h`, gleiches Präfix wie die App). Grund: `app/debug_log` liegt in `app`, und `settings` darf nur von `core` abhängen.
- 2026-11-06 (M3): **Vorlauf und Einblenden beim Start.** `Snowfall::prewarm()` simuliert vor dem ersten Frame, bis die langsamsten Flocken einmal über den Bildschirm gefallen sind (2× 1440p: 37 s Simulationszeit, rund 40 ms im Debug-Build). Danach wird der Schnee über 2 s eingeblendet, über einen globalen Faktor `opacity` im Konstantenpuffer. Grund: Vorgabe von Lars, der Schnee soll sofort gleichmäßig verteilt sein und weich erscheinen. Der Faktor dient später auch den Modusübergängen (M7).
- 2026-11-06 (M3): **Seitlicher Wrap.** Flocken, die den virtuellen Bildschirm seitlich verlassen, kommen knapp außer Sicht am gegenüberliegenden Rand wieder herein. Grund: Ohne Wrap verarmt bei anhaltendem Wind der windzugewandte Rand. Der Skill nannte dafür keine Regel.
- 2026-11-06 (M3): **Spawn-Rate in Flocken pro Sekunde und 1000 px** abgedeckter Bildschirmbreite (Leichter Schnee: 8, etwa 900 Flocken auf 2× 2560 px) statt „pro Megapixel-Breite“. Grund: Die frühere Einheit war uneindeutig.
- 2026-11-06 (M3): **Drei Tiefenstufen als Bänder von `depth`** ([0,⅓), [⅓,⅔), [⅔,1], Gewichte 50/30/20 %). Größe, Fallgeschwindigkeit, Alpha, Windanteil und Wobble folgen linear aus `depth`, Radius und Alpha streuen zusätzlich pro Flocke. Offen für M5: ob ab der mittleren oder nur die vordere Stufe kollidiert.
- 2026-11-06 (M3): **Kantenglättung der Flocken in Pixeleinheiten** statt `smoothstep(0.6, 1.0, d)` auf dem Einheitsquad. Das Quad ist 1 px größer als der Radius, die weiche Kante mindestens etwa 1 px breit, Radien unter 1 px werden mit 1 px und entsprechend geringerer Alpha gezeichnet. Grund: Kleine Flocken flackerten sonst bei Sub-Pixel-Bewegung.
- 2026-11-06 (M3): **Accumulator mit halbem Tick Phasenversatz**, ganzzahlig in QPC-Ticks. Grund: Bei 60 FPS und 60 Hz Simulation lägen Tick- und Frame-Grenzen sonst übereinander, und Wake-up-Jitter ergäbe abwechselnd 0 und 2 Ticks pro Frame (Ruckeln). Frames ohne fälligen Tick werden nicht gerendert.
- 2026-11-06 (M3): Die Klasse für den Flockenfall heißt **`Snowfall`**. `World` aus dem Überblick entsteht erst mit Fenster-Tracking und Schneedecken (M4/M5) und enthält dann `Snowfall`.
- 2026-11-06 (M3): Gemessen (x64 Release, 2× 1440p, 60 FPS, 120 s): Leichter Schnee mit rund 900 Flocken 0,12 % CPU (3,7 % eines Kerns von 32), 33,9 MB Working Set; voller Pool mit 4000 Flocken 0,14 % CPU (4,4 % eines Kerns). Der Großteil ist fixer Aufwand pro Frame (Present, Composition), nicht die Simulation.
<!-- Weitere Abweichungen und Begründungen hier eintragen (Datum, Entscheidung, Grund) -->
