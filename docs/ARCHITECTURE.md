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
- Ein unsichtbares **Message-Only-Fenster** (`HWND_MESSAGE`) empfängt Tray-Nachrichten, `TaskbarCreated` und Hotkeys.
- `TrayIcon`: `Shell_NotifyIconW` mit Version 4. Kontextmenü **Einstellungen** und **Beenden**.
- `SettingsWindow` (Modul `src/settings`): Win32-Fenster mit **WinUI 3 über XAML Islands** (Windows App SDK, C++/WinRT). Es wird bei Bedarf erzeugt und beim Schließen zerstört, höchstens eine Instanz. Die WinUI-Laufzeit wird erst beim ersten Öffnen geladen. Inhalt vorerst die Info-Seite. Es ist das einzige Fenster der App mit Taskleisten-Eintrag, und auch das nur, solange es offen ist.

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
<!-- Weitere Abweichungen und Begründungen hier eintragen (Datum, Entscheidung, Grund) -->
