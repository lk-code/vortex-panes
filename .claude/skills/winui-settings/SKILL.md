---
name: winui-settings
description: Use when working on the settings window – real WinUI 3 hosted in a Win32 window via XAML Islands (DesktopWindowXamlSource, Windows App SDK, C++/WinRT), integrating the Windows App SDK and C++/WinRT NuGet packages into the CMake build, lazy loading, theming/Mica, localization of XAML content, accessibility and self-contained vs. framework-dependent deployment.
---

# Einstellungsfenster: WinUI 3 über XAML Islands

**Ziel:** echtes WinUI 3 im Look der Windows-11-Einstellungen, in einer Win32-App ohne WinUI-`Application`. Das Overlay ist davon unberührt.

> **Wichtig:** Die APIs rund um Islands haben sich zwischen den Versionen des Windows App SDK verändert. Vor dem Umsetzen immer die **aktuelle** Doku auf Microsoft Learn und die offiziellen Beispiele (`microsoft/WindowsAppSDK-Samples`, Bereich Islands) gegen die verwendete SDK-Version prüfen. Die Angaben hier sind Leitplanken, keine wörtliche API-Referenz.

## Spike zu Beginn von M2 (Pflicht, danach Stopp)
1. Aktuelle **stabile** Version von `Microsoft.WindowsAppSDK` und `Microsoft.Windows.CppWinRT` auf nuget.org ermitteln. Kein Preview und kein Experimental.
2. **Lizenzen prüfen**, und zwar für den Quellcode, die NuGet-Pakete und die mitgelieferten Laufzeit-Binärdateien (auch `Microsoft.Windows.SDK.BuildTools` und andere transitive Pakete). Ergebnis als Tabelle: Paket, Version, Lizenz, ausgeliefert ja/nein, Weitergabe erlaubt ja/nein.
3. **Build-Einbindung** prototypisch lösen. Bevorzugt bleibt CMake erhalten:
   - Pakete per CMake beziehen (nuget.org-`.nupkg` sind ZIPs: `file(DOWNLOAD)` mit fester Version **und** SHA256-Hash, dann entpacken). Alternativ `nuget.exe restore` als Schritt vor dem Configure.
   - `.props`/`.targets` der Pakete per `VS_PROJECT_IMPORT` (Target-Property) ins generierte `.vcxproj` importieren. Damit laufen die C++/WinRT-Projektionserzeugung und die WinAppSDK-Deployment-Logik.
   - Nur falls das nicht sauber funktioniert: Vorschlag machen, das Settings-Modul oder das ganze Projekt auf MSBuild-`.vcxproj` umzustellen, mit Vor- und Nachteilen. **Nicht eigenmächtig umbauen.**
4. Bericht an mich mit dem Weg, der Lizenz-Tabelle, der Größe des App-Ordners pro Architektur und offenen Risiken. Danach warten.

## Architektur
- Das Fenster ist ein eigenes Win32-Top-Level-Fenster (`WS_OVERLAPPEDWINDOW` ohne Maximieren, Startgröße ca. 720×560 DIP, Mindestgröße gesetzt), App-Icon, Titel aus der STRINGTABLE.
- Pro Thread einmalig, und zwar erst beim ersten Öffnen:
  - Windows App Runtime initialisieren. Bei Self-Contained erledigt das die Deployment-Logik des SDK. Bei framework-abhängig ohne Paket den Bootstrapper verwenden, im MSIX die Paketabhängigkeit.
  - `DispatcherQueueController` für den UI-Thread anlegen, falls noch keiner existiert
  - XAML für den Thread initialisieren (`WindowsXamlManager`)
- `DesktopWindowXamlSource` an das Fenster hängen (über dessen `WindowId`), `Content` setzen, das Island bei `WM_SIZE` auf die Client-Größe bringen.
- **Message-Loop:** Die Main-Loop muss Tastatur-Nachrichten an das Island weiterreichen (Pre-Translate für Tab, Pfeile, Mnemonics). Die passende API für die verwendete SDK-Version aus der Doku nehmen. Ohne diesen Schritt funktionieren Tab-Navigation und Barrierefreiheit nicht.
- **Kein zweiter Thread** für die UI. Alles läuft auf dem bestehenden UI-Thread, das Overlay rendert dort ebenfalls und blockiert nicht.
- Schließen: Island und Fenster zerstören. Die WinUI-Initialisierung des Threads bleibt bestehen, beim erneuten Öffnen wird nur das Fenster neu erzeugt.

## Lazy Loading (Performance-Pflicht)
- WinAppSDK- und WinUI-DLLs werden per **`/DELAYLOAD`** gelinkt, wo möglich. Die gesamte WinUI-Initialisierung liegt in einem eigenen Modul (`src/settings/`), das erst `SettingsWindow::show()` anfasst.
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
- **Mica** unter Windows 11. Die Variante für die SDK-Version aus der Doku nehmen: System-Backdrop am Island oder per DWM (`DWMWA_SYSTEMBACKDROP_TYPE`) mit transparentem XAML-Root. Unter Windows 10 gibt es einen soliden Hintergrund (`SolidBackgroundFillColorBaseBrush`).

## DPI und Fenster
- Das Host-Fenster ist PerMonitorV2. WinUI skaliert selbst. Bei `WM_DPICHANGED` das vorgeschlagene Rect übernehmen und das Island neu dimensionieren.
- Fenstergröße und -position werden **nicht** gespeichert, das kommt eventuell später.

## Deployment
| Kanal | Modus | Folge |
|---|---|---|
| Portable-ZIP (GitHub) | **Self-Contained** (`WindowsAppSDKSelfContained=true`) | WinAppSDK-DLLs und `.pri` liegen im App-Ordner. Es braucht keine Installation, der Ordner ist größer. `install()`-Regeln müssen alles mitnehmen. |
| MSIX (Store, M10) | **Framework-abhängig** | `PackageDependency` auf die Windows App Runtime. Der Store installiert sie automatisch, das Paket bleibt klein. |
- Beide Varianten aus derselben Codebasis, umgeschaltet per CMake-Option (`VP_WINAPPSDK_SELF_CONTAINED`, Standard `ON`).
- **Unterstützte Architekturen:** x86, x64 und ARM64 werden vom Windows App SDK unterstützt. Das in M2 in der CI für alle drei verifizieren.
- Mindest-Windows ist die höhere Anforderung aus unserer (Windows 10 21H2) und der des Windows App SDK.

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
