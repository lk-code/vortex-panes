---
name: winui-settings
description: Use when working on the settings window – real WinUI 3 hosted in a Win32 window via XAML Islands (DesktopWindowXamlSource, Windows App SDK, C++/WinRT), integrating the Windows App SDK and C++/WinRT NuGet packages into the CMake build, lazy loading, theming/Mica, localization of XAML content, accessibility and self-contained vs. framework-dependent deployment.
---

# Einstellungsfenster: WinUI 3 über XAML Islands

**Ziel:** echtes WinUI 3 im Look der Windows-11-Einstellungen, in einer Win32-App ohne das WinUI-App-Modell (`Application::Start`). Für Islands ist trotzdem eine eigene `Application`-Unterklasse nötig (Metadaten und Fluent-Ressourcen). Das Overlay ist davon unberührt.

**Stand M2 (2026-11-06):** Windows App SDK 2.6.5 (nur WinUI-Komponenten), C++/WinRT 3.0.260818.1, umgesetzt in `src/settings` und `cmake/WinAppSdk.cmake`. Entscheidungen und Gründe stehen in `docs/ARCHITECTURE.md`.

> **Wichtig:** Die APIs rund um Islands haben sich zwischen den Versionen des Windows App SDK verändert. Vor dem Umsetzen immer die **aktuelle** Doku auf Microsoft Learn und die offiziellen Beispiele (`microsoft/WindowsAppSDK-Samples`, Bereich Islands) gegen die verwendete SDK-Version prüfen. Die Angaben hier sind Leitplanken, keine wörtliche API-Referenz.

## Build-Einbindung (`cmake/WinAppSdk.cmake`, Ergebnis des M2-Spikes)
- **Pakete:** `file(DOWNLOAD)` von nuget.org mit fester Version **und** SHA256, entpackt nach `out/nuget` (für alle Presets gemeinsam, in der CI per `actions/cache`). Verwendet werden `Microsoft.WindowsAppSDK.WinUI`, `.InteractiveExperiences`, `.Foundation`, `.Base`, dazu `Microsoft.Web.WebView2` (nur Metadaten für die Projektion) und `Microsoft.Windows.CppWinRT`. `Microsoft.WindowsAppSDK.Runtime` kommt nur bei `VP_WINAPPSDK_SELF_CONTAINED=OFF` dazu. Nicht das Metapaket (AI/ML, ONNX).
- **Update der Versionen:** Nur stabile Versionen. Die Abhängigkeitsversionen aus den `.nuspec`-Dateien übernehmen (NuGet nimmt die niedrigste passende Version), SHA256 neu berechnen und einmal gegen den `packageHash` (SHA512) im nuget.org-Katalog prüfen. Lizenztexte in den Paketen auf Änderungen prüfen und `THIRD_PARTY_NOTICES.md` anpassen.
- **Projektion:** `cppwinrt.exe -input <SDK-Version> -input <winmds> -output <build>/cppwinrt` beim Konfigurieren, als SYSTEM-Include im Interface-Target `vp_winappsdk`. Linken: `runtimeobject` (nicht `WindowsApp.lib`, das die kernel32-Importe auf API-Sets umbiegt). Die CppWinRT-MSBuild-Targets werden nicht verwendet.
- **Deployment:** `.props`/`.targets` der Komponenten per `VS_PROJECT_IMPORT` nur in das Exe-Projekt (`vp_winappsdk_deploy`). Die offiziellen Targets kopieren die Laufzeit neben die Exe und mergen die RegFree-WinRT-Einträge (`activatableClass`) in das Anwendungsmanifest. `cmake/winappsdk_overrides.targets` entfernt unnötige `.winmd`-Referenzen (MSB3106), stuft CS1668 herab und leert `_OnXamlPreCompileError` (sonst falsches MSB4044 nach jedem Compilerfehler).
- **Ressourcenindex:** `makepri` aus dem Windows SDK mergt die `.pri`-Dateien der Komponenten zu `VortexPanes.pri` (`cmake/priconfig.xml`). Ohne diese Datei findet WinUI unpaketiert seine Themenressourcen nicht.
- **install():** dieselben Dateimuster wie `Microsoft.WindowsAppSDK.SelfContained.targets` (DLLs, `.mui`, `.pri` usw., ohne `.winmd`), dazu `VortexPanes.pri` und `licenses/`.

## Architektur
- Das Fenster ist ein eigenes Win32-Top-Level-Fenster (`WS_OVERLAPPEDWINDOW` ohne Maximieren, Startgröße ca. 720×560 DIP, Mindestgröße gesetzt), App-Icon, Titel aus der STRINGTABLE.
- Pro Thread einmalig, und zwar erst beim ersten Öffnen (`settings/xaml_runtime`), in dieser Reihenfolge:
  - `CoInitializeEx(COINIT_APARTMENTTHREADED)`
  - `DispatcherQueueController::CreateOnCurrentThread()`
  - eigene `Application`-Unterklasse (`ApplicationT` + `IXamlMetadataProvider`, delegiert an `XamlControlsXamlMetaDataProvider`). Ihr Konstruktor ruft `WindowsXamlManager::InitializeForCurrentThread()`, dabei kommt `OnLaunched`, und dort wird `XamlControlsResources` gemergt.
  - Self-Contained braucht keinen weiteren Schritt (RegFree-WinRT über das Manifest, alle Auto-Initializer aus). Framework-abhängig ohne Paket bräuchte den Bootstrapper, im MSIX (M10) genügt die Paketabhängigkeit.
- `DesktopWindowXamlSource::Initialize(GetWindowIdFromWindow(hwnd))`, `Content` setzen, das Island bei `WM_SIZE` über `SiteBridge().MoveAndResize` auf die Client-Größe bringen.
- **Message-Loop:** Die Main-Loop gibt jede Nachricht vor `TranslateMessage` an `ContentPreTranslateMessage` (Export von `Microsoft.UI.Windowing.Core.dll`, per `GetProcAddress` nach der Initialisierung aufgelöst). Ohne diesen Schritt funktionieren Tab-Navigation und Zugriffstasten nicht.
- **Fokus:** Bei `WM_ACTIVATE`/`WM_SETFOCUS` den Win32-Fokus auf das Island-Kindfenster legen (`GetWindowFromWindowId(SiteBridge().WindowId())`) und `NavigateFocus` aufrufen (beim ersten Mal `First`, danach `Restore`). Vor `Loaded` der Wurzel bewegt `NavigateFocus` nichts. `TakeFocusRequested` mit `NavigateFocus(reason)` beantworten, damit Tab im Fenster umläuft, aber Anfragen während eines eigenen `NavigateFocus`-Aufrufs ignorieren. Sonst gibt es eine Endlosrekursion und einen Stack-Overflow.
- **Kein zweiter Thread** für die UI. Alles läuft auf dem bestehenden UI-Thread, das Overlay rendert dort ebenfalls und blockiert nicht.
- Schließen: Island und Fenster zerstören. Die WinUI-Initialisierung des Threads bleibt bestehen, beim erneuten Öffnen wird nur das Fenster neu erzeugt.

## Lazy Loading (Performance-Pflicht)
- Die Exe importiert **keine** WinAppSDK-DLL statisch (mit `dumpbin /dependents` prüfen). C++/WinRT aktiviert über `RoGetActivationFactory`, die Interop-Helfer laden ihre DLL selbst, `ContentPreTranslateMessage` per `GetProcAddress`. `/DELAYLOAD` ist deshalb nicht nötig. Die gesamte WinUI-Initialisierung liegt in `src/settings/`, das erst `SettingsWindow::show()` anfasst.
- Prüfen (in der Meilenstein-Zusammenfassung aufzeigen): Nach dem Start ohne Öffnen der Einstellungen ist keine `Microsoft.UI.Xaml.dll` geladen (Process Explorer bzw. `tasklist /m`).

## UI-Inhalt
- XAML als **eingebettete Ressource** (`RCDATA`, UTF-8) und zur Laufzeit per `Microsoft::UI::Xaml::Markup::XamlReader::Load` geladen. So braucht es keinen XAML-Compiler und keine generierten Code-Behind-Dateien im CMake-Build. Elemente werden per `x:Name` + `FindName` angesprochen, Events im C++-Code verdrahtet.
- Das XAML enthält **keine Texte**. Alle Texte setzt der Code aus der STRINGTABLE (7 Sprachen, siehe Skill `app-shell`).
- Aufbau der Info-Seite im Stil der Windows-11-Einstellungen:
  - Kopf: App-Icon 64 px, App-Name (`TitleTextBlockStyle`), darunter „Version x.y.z (x64)“ (`CaptionTextBlockStyle`, sekundäre Farbe)
  - „Settings-Cards“ (Border mit `CardBackgroundFillColorDefaultBrush`, `CornerRadius 8`, Padding 16, Abstand 4): Entwickler → „lk-code“, Webseite → `HyperlinkButton` auf `VP_WEBSITE`, Lizenzen von Drittanbietern → `HyperlinkButton`, öffnet einen `ContentDialog` mit `THIRD_PARTY_NOTICES` in einem scrollbaren, schreibgeschützten `TextBlock`
  - Nur Theme-Ressourcen (`{ThemeResource …}`) verwenden, keine festen Farben, damit Hell, Dunkel und Kontrastmodi stimmen.
- **Erweiterbar:** Den Inhalt jetzt schon in einen Seiten-Container legen. In M8 kommt eine `NavigationView` (links, `PaneDisplayMode=Left`, Fenster etwas breiter, ca. 900×640 DIP) mit den Seiten **„Darstellung“** und **„Info“** dazu. Später folgen weitere, z. B. „Allgemein“ und „Wetter“.

## Seite „Darstellung“ (ab M8)
- Überschrift „Hintergrund für Regentropfen“ plus eine kurze Erklärung (die Tropfen zeigen dieses Bild wie durch eine Linse)
- **Galerie** (`GridView`, Kacheln ca. 200×125, abgerundet, Bildname darunter):
  - alle mitgelieferten Bilder (Thumbnails per WIC aus der eingebetteten Ressource erzeugen, als `SoftwareBitmapSource` oder `WriteableBitmap` setzen, kein Temp-Ordner)
  - falls vorhanden, das eigene Bild als Kachel „Eigenes Bild“ mit Entfernen-Button (Mülleimer-Symbol, Tooltip)
  - eine letzte Kachel **„Eigenes Bild …“** mit Plus-Symbol
  - Die Auswahl wird per `SelectionChanged` sofort angewendet und gespeichert, das Overlay lädt die Textur neu.
- **Dateiauswahl:** Win32 `IFileOpenDialog` (COM, Teil von Windows) mit dem Settings-Fenster als Owner. Filter „Bilder“ (`*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.tif;*.tiff;*.webp;*.heic`) und „Alle Dateien“. Kein `FileOpenPicker`, der braucht im unpaketierten Fall Extra-Initialisierung. Verarbeitung (Dekodieren, Skalieren, Speichern) asynchron, damit die UI nicht hängt. Währenddessen erscheint ein `ProgressRing` auf der Kachel.
- **Vorschau** rechts bzw. unter der Galerie: das gewählte Bild als Hintergrund und darüber 5–8 Beispiel-Tropfen, statisch aus demselben Shader-Code gerendert (eigene kleine D3D-Textur → Bitmap) oder vereinfacht als XAML-Ellipsen mit Bildausschnitt. Statisch reicht.
- Fehler beim Laden: `InfoBar` (Severity Error) mit verständlichem Text, die alte Auswahl bleibt.

## Theme und Backdrop
- Hell und Dunkel folgen automatisch dem App-Theme von Windows. Bei `WM_SETTINGCHANGE` mit `ImmersiveColorSet` gegebenenfalls `RequestedTheme` des Root-Elements aktualisieren.
- Titelleiste des Host-Fensters: `DwmSetWindowAttribute(DWMWA_USE_IMMERSIVE_DARK_MODE)` passend zum Theme.
- **Mica:** `DesktopWindowXamlSource::SystemBackdrop(MicaBackdrop())`, und zwar **erst nach** `Content` (vorher Zugriffsverletzung in `Microsoft.UI.Xaml.dll`). Meldet `MicaController::IsSupported()` false, bekommt die Wurzel `SolidBackgroundFillColorBaseBrush`.

## DPI und Fenster
- Das Host-Fenster ist PerMonitorV2. WinUI skaliert selbst. Bei `WM_DPICHANGED` das vorgeschlagene Rect übernehmen und das Island neu dimensionieren.
- Fenstergröße und -position werden **nicht** gespeichert, das kommt eventuell später.

## Deployment
| Kanal | Modus | Folge |
|---|---|---|
| Portable-ZIP (GitHub) | **Self-Contained** (`WindowsAppSDKSelfContained=true`) | WinAppSDK-DLLs und `.pri` liegen im App-Ordner. Es braucht keine Installation, der Ordner ist größer. `install()`-Regeln müssen alles mitnehmen. |
| MSIX (Store, M10) | **Framework-abhängig** | `PackageDependency` auf die Windows App Runtime. Der Store installiert sie automatisch, das Paket bleibt klein. |
- Beide Varianten aus derselben Codebasis, umgeschaltet per CMake-Option (`VP_WINAPPSDK_SELF_CONTAINED`, Standard `ON`).
- **Unterstützte Architekturen:** x86, x64 und ARM64 werden vom Windows App SDK unterstützt. Größe des App-Ordners (M2, self-contained): x86 ca. 51 MB, x64 ca. 55 MB, ARM64 ca. 62 MB, als ZIP jeweils ca. 19–21 MB. Keine ausgelieferte DLL braucht die VC++-Redist (Hybrid-CRT von Microsoft, unsere Exe hat die statische CRT).
- Mindest-Windows: Windows 11 21H2 (Build 22000), Entscheidung M2. Das Windows App SDK selbst unterstützt ab Windows 10 1809.
- **Test ohne Runtime:** Windows Sandbox mit schreibgeschützt eingebundenem App-Ordner. Die Sandbox hat keine reguläre Windows App Runtime installiert, nur die `…CBS`-Pakete von Windows selbst.

## Barrierefreiheit
- WinUI-Controls bringen UI Automation mit. Zusätzlich `AutomationProperties.Name` für Icon und Links aus der STRINGTABLE setzen.
- Manuell prüfen: komplette Bedienung per Tastatur (Tab, Enter, Esc schließt), Narrator liest alle Elemente vor, Kontrastdesign von Windows.

## Tests
- Logik ohne UI (Versions- und Architektur-String, Auswahl der Texte, Aufbau der Notices) mit GoogleTest
- UI manuell, die Checkliste kommt in die Meilenstein-Zusammenfassung:
  - erstes und erneutes Öffnen
  - Theme-Wechsel bei offenem Fenster
  - 100/150/200 % DPI und Monitorwechsel
  - alle 7 Sprachen
  - Tastatur und Narrator
  - frisches Windows ohne installierte Runtime (Portable-ZIP)
