---
name: win32-window-tracking
description: Use when working on src/platform – enumerating windows, z-order, occlusion of top edges, taskbar detection, DPI, WinEvent hooks, cursor/monitor sources or fullscreen detection.
---

# Win32 Window Tracking

## Fenster aufzählen
- `EnumWindows` liefert Top-Level-Fenster **in Z-Order von oben nach unten**. Diese Reihenfolge wird direkt als Z-Index übernommen.
- Ein Fenster gilt als **relevant**, wenn **alle** Bedingungen erfüllt sind:
  - `IsWindowVisible(h)` ist true
  - `IsIconic(h)` ist false (nicht minimiert)
  - `DwmGetWindowAttribute(h, DWMWA_CLOAKED = 14)` ist 0. Damit fallen UWP-Geisterfenster und Fenster auf anderen virtuellen Desktops raus.
  - Es ist nicht das eigene Overlay. Die eigenen HWNDs stehen in einer kleinen festen Liste, das sind maximal so viele wie Monitore.
  - Es hat kein `WS_EX_TOOLWINDOW`, außer es ist eine Taskleiste. Das filtert Tooltips und Popups heraus.
  - `GetWindowLong(GWL_EXSTYLE) & WS_EX_TRANSPARENT` ist 0.
  - Der Klassenname ist nicht `Progman`, `WorkerW`, `Windows.UI.Core.CoreWindow` (Startmenü, Suche), `NotifyIconOverflowWindow` oder `Xaml_WindowedPopupClass`.
  - Breite und Höhe sind jeweils ≥ 50 px.
- Die Filterlogik kommt als **reine Funktion** `isRelevant(const WindowInfo&)` in eine testbare Einheit. Die Win32-Aufrufe sammeln nur die Rohdaten in `WindowInfo`.

## Rechteck
- **Nicht** `GetWindowRect` verwenden. Es enthält unsichtbare Resize-Ränder von etwa 7 px. Stattdessen `DwmGetWindowAttribute(h, DWMWA_EXTENDED_FRAME_BOUNDS = 9, out RECT, sizeof(RECT))`.
- Die Prozess-DPI-Awareness ist **PerMonitorV2** (Manifest). Damit sind alle Koordinaten physische Pixel im virtuellen Bildschirm, und so bleibt es überall in `core`.

## Taskleiste
- Die Hauptleiste hat die Klasse `Shell_TrayWnd`, Zusatzmonitore haben `Shell_SecondaryTrayWnd`. Beide sind `Kind = Taskbar`.
- Die Position kann unten, oben oder seitlich sein. Schnee gibt es nur, wenn sie unten liegt, sonst wird sie als Fläche ignoriert.
- Automatisch ausgeblendete Taskleiste: Wenn ihr Rechteck größtenteils außerhalb des Monitors liegt, wird sie ignoriert. Der Boden ist dann die Monitor-Unterkante.

## Verdeckung (OcclusionCalculator)
- Eingabe: Flächen sortiert nach Z-Index. Ausgabe: pro Fläche eine Liste von `(int x0, int x1)` sichtbarer Abschnitte ihrer Oberkante.
- Algorithmus: Starte für Fläche *i* mit dem Intervall `[left, right)`. Der Schnee liegt im Band `[top - MaxSnowHeight, top]`. Für jede Fläche *j* mit Z-Index < i, deren Rechteck dieses Band vertikal schneidet (`top_j <= top && bottom_j > top - MaxSnowHeight`), wird `[left_j, right_j)` vom Intervall abgezogen. So wird auch Schnee ausgeblendet, der sonst über ein knapp darüberliegendes Fenster ragen würde.
- Zusätzlich wird auf die Monitor-Rechtecke geclippt. Oberkanten auf `y == monitor.Top` (maximierte Fenster) bekommen keine Segmente.
- O(n²) ist bei unter 100 Fenstern völlig okay. Hier nicht vorzeitig optimieren.
- **Ausführlich testen:** keine Überlappung, teilweise Überlappung links/rechts/mittig, komplett verdeckt, mehrere Verdecker, Fenster über zwei Monitore.

## Änderungen erkennen
- `SetWinEventHook(EVENT_SYSTEM_MOVESIZESTART .. EVENT_OBJECT_LOCATIONCHANGE, …, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS)`. Relevant sind: `EVENT_OBJECT_LOCATIONCHANGE` (0x800B, nur `idObject == OBJID_WINDOW`), `EVENT_SYSTEM_FOREGROUND`, `EVENT_SYSTEM_MINIMIZESTART/END`, `EVENT_OBJECT_DESTROY`, `EVENT_OBJECT_SHOW/HIDE`, `EVENT_OBJECT_CLOAKED/UNCLOAKED`.
- Der Hook setzt nur ein Dirty-Flag. Der Snapshot wird **im Tick** neu aufgebaut, nicht im Callback.
- Zusätzlich wird alle 100 ms neu aufgezählt, als Fallback für verpasste Events.
- Hook-Handles in einem RAII-Wrapper halten, der beim Beenden `UnhookWinEvent` aufruft.
- Der Hook braucht eine Message-Loop auf dem registrierenden Thread. Die Main-Loop (`MsgWaitForMultipleObjectsEx` + `PeekMessage`) liefert sie.

## Cursor und Monitore
- Cursor: `GetCursorPos` in jedem Tick. Die Geschwindigkeit wird aus der Differenz berechnet und geglättet (EMA).
- Monitore: `EnumDisplayMonitors` + `GetMonitorInfo` (rcMonitor, rcWork) + `GetDpiForMonitor`. Bei `WM_DISPLAYCHANGE` oder `WM_DPICHANGED` werden die Overlays neu aufgebaut.

## Vollbild-Erkennung
- `SHQueryUserNotificationState`: `QUNS_RUNNING_D3D_FULL_SCREEN` oder `QUNS_PRESENTATION_MODE` bedeutet pausieren.
- Zusätzlich gilt: Wenn das Vordergrundfenster seinen Monitor exakt füllt und kein Desktop (`Progman`/`WorkerW`) ist, wird auf diesem Monitor pausiert.
- Bei gesperrter Session pausieren: `WTSRegisterSessionNotification` mit `WTS_SESSION_LOCK/UNLOCK`.

## Code-Stil (C++)
- Zweiteilung für die Testbarkeit:
  - `collectWindowInfos(std::vector<WindowInfo>& out)` erledigt nur die Win32-Aufrufe und füllt Rohdaten (HWND, Klassenname, Styles, Rect, Cloaked, Iconic, Visible).
  - `isRelevant(const WindowInfo&)` und `computeVisibleTopSegments(…)` sind **reine Funktionen** ohne `<windows.h>` im Header und deshalb mit GoogleTest testbar.
- Puffer (`std::vector<WindowInfo>`, Segmentlisten) als Member mit `reserve(256)`, pro Snapshot nur `clear()`.
- Klassennamen per `GetClassNameW` in einen festen `wchar_t[64]`-Puffer, Vergleich mit `wcscmp`. Keine `std::wstring`-Allokation pro Fenster.
- `EnumWindows`-Callback als `static BOOL CALLBACK` mit `this` über `LPARAM`.
- WinEvent-Callback als freie Funktion. Sie greift über einen `static`-Zeiger auf den Tracker zu, weil es nur eine Instanz gibt, und setzt nur `m_dirty = true`.
