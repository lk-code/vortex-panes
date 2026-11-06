# Roadmap

Jeder Meilenstein endet mit: Build ohne Warnungen (Debug und Release), Tests grün, kurze Anleitung für den manuellen Test, **Stopp für Review**.

## M1: Grundgerüst, Multi-Arch-Build und GPU-Overlay
- VS-Version lokal und auf den GitHub-Runnern prüfen, danach den Generator im Basis-Preset setzen (Skill `build-and-release`)
- `CMakeLists.txt`, `CMakePresets.json` (x86/x64/arm64), Targets laut `CLAUDE.md`, GoogleTest via FetchContent
- Git-Remote auf https://github.com/lk-code/vortex-panes setzen (nur nach meinem OK pushen)
- Repo-Grundlagen: **`.gitignore` passend zu C++/CMake/MSVC/Windows App SDK/MSIX** (Vorgaben im Skill `build-and-release`), `LICENSE` (MIT, „Copyright (c) 2026 lk-code“), `THIRD_PARTY_NOTICES.md` (vorerst: „keine ausgelieferten Fremdkomponenten“, GoogleTest als Test-only), `.gitignore`, `.gitattributes`, `.editorconfig`, `.clang-format`
- `.github/workflows/ci.yml` ist bereits vorhanden. Bei Bedarf an die echten Pfade anpassen, die CI muss für alle drei Architekturen grün sein.
- `app.manifest` (PerMonitorV2, Common Controls 6), `app.rc` mit den vorhandenen Icons aus `resources/`, `VERSIONINFO`, Sprachdateien für **7 Sprachen** (en, de, es, fr, zh-CN, ja, ko) inkl. `tools/Check-Strings.ps1` in der CI
- **App-Shell** (Skill `app-shell`): Start ohne Taskleisten-Fenster, Tray-Icon (hell/dunkel nach Taskleisten-Theme) mit Kontextmenü **Einstellungen** / **Beenden**, Single-Instance. „Einstellungen“ ist bis M2 ausgegraut.
- Shader-Pipeline: `fxc` als CMake-Custom-Command, erzeugt Header mit Bytecode
- Ein Overlay pro Monitor: Click-Through-Fenster, D3D11 + DComp-Swapchain, Alpha premultiplied
- Main-Loop mit High-Res-Waitable-Timer und FPS-Limit
- Testinhalt: ein halbtransparentes Rechteck und ein weicher Kreis pro Monitor
- Debug-Build: FPS und Frame-Zeit im Fenstertitel eines versteckten Debug-Fensters oder per `OutputDebugString`
- **Abnahme:** Kein Eintrag in der Taskleiste, nur das Tray-Icon. „Beenden“ beendet sauber. Das Overlay ist auf allen Monitoren sichtbar, Klicks gehen durch, es taucht nicht in Alt-Tab auf, die CPU liegt nahe 0 %. Lokal bauen alle drei Architekturen, die CI ist grün.

## M2: Einstellungsfenster mit WinUI 3 (XAML Islands)
- **Spike zuerst** (Skill `winui-settings`): aktuelle stabile Windows-App-SDK-Version ermitteln, Lizenzen aller benötigten NuGet-Pakete prüfen und mir berichten, Einbindung in CMake prototypisch lösen. **Danach Stopp und Rückfrage**, bevor der Spike in die Codebasis übernommen wird. Das gilt besonders, falls der Wechsel von CMake auf MSBuild (`.vcxproj`) nötig erscheint.
- Einstellungsfenster als Win32-Fenster mit `DesktopWindowXamlSource`, Inhalt per `XamlReader::Load` aus eingebettetem XAML
- Info-Seite: Icon, App-Name, Version mit Architektur, Entwickler (lk-code), Link zur Webseite, Link zu den Drittanbieter-Lizenzen. Alle Texte in 7 Sprachen.
- Hell/Dunkel folgt Windows, Mica unter Windows 11, sauberer Fallback unter Windows 10
- WinUI wird **erst beim ersten Öffnen** geladen. Ohne geöffnetes Fenster ändern sich weder CPU noch RAM.
- Deployment: Portable-ZIP self-contained (WinAppSDK-DLLs neben der Exe), MSIX später framework-abhängig. `cmake --install` legt den kompletten Ordner für die ZIP an. CI und Release-Workflow werden darauf umgestellt.
- `THIRD_PARTY_NOTICES.md` um das Windows App SDK und alle mitgelieferten Komponenten ergänzen
- **Abnahme:** Das Fenster sieht aus wie eine Windows-11-Einstellungsseite (Mica, Fluent-Controls, Dark Mode). Tastatur und Screenreader (Narrator) funktionieren. Es läuft auf x86, x64 und ARM64 in der CI. Die Portable-ZIP startet auf einem frischen Windows ohne Zusatzinstallation. Ohne geöffnetes Fenster liegt der RAM wie vorher.

## M3: Schneefall
- `Flake`, `FlakePool`, RNG, Wind (sanftes Rauschen), Wobble, Tiefenstufen (Parallax)
- Instanced Rendering der Flocken mit weichem Kreis im Pixel-Shader
- Boden vorerst: Unterkante des Monitors
- **Tests:** Pool, Integrator, Determinismus (gleicher Seed, gleiches Ergebnis)
- **Abnahme:** Natürlich wirkender Schneefall, CPU < 1 %

## M4: Fenster-Tracking
- `WindowTracker` (EnumWindows, Filterregeln, `DWMWA_EXTENDED_FRAME_BOUNDS`, Cloaked-Check, Taskleisten)
- `SetWinEventHook` plus Polling mit 10 Hz als Fallback
- Verdeckungsberechnung der Oberkanten
- Debug-Ansicht (Setting oder Hotkey): sichtbare Segmente als farbige Linien
- **Tests:** Filterregeln (aus Rohdaten), Verdeckung (überlappend, enthalten, mehrfach, über Monitorgrenzen)
- **Abnahme:** Die Debug-Linien sitzen exakt auf den sichtbaren Fensterkanten und der Taskleiste und folgen Bewegungen ohne Verzögerung.

## M5: Schnee sammelt sich
- `SnowCap` pro Fläche: Ablagern, Abrutschen (Schüttwinkel), Maximum, Randüberlauf, Schmelzen
- Boden: Taskleiste bzw. Monitor-Unterkante sammelt ebenfalls
- Rendern der Decke als geglättetes Profil mit dezentem Schatten, nur in sichtbaren Segmenten
- **Tests:** Ablagern, Masseerhaltung beim Abrutschen, Maximum, Schmelzen, Randspalten
- **Abnahme:** Weiche Schneeschicht auf Fenstern und Taskleiste wie im Referenzvideo

## M6: Interaktion
- Langsam verschieben: Schnee wandert mit
- Fling, Resize, Minimieren, Schließen: Schnee fällt bzw. fliegt ab
- Cursor-Brush
- **Tests:** Ereigniserkennung aus Snapshot-Folgen, Brush
- **Abnahme:** Alle drei Effekte aus dem Video funktionieren.

## M7: Wettermodi und Sonne
- Wettersystem laut Skill `weather-and-rain`: Modi **Leichter Schnee** (Standard), **Schneesturm**, **Sonne**, **Leichter Regen**, **Regensturm**
- Weiche Übergänge beim Moduswechsel (Ein- und Ausblenden über einige Sekunden, kein harter Schnitt)
- **Sonne:** Es fällt nichts mehr, liegender Schnee taut ab und Tropfen verdunsten, beides gleichmäßig über **60 s**. Vom oberen Rand jedes Monitors scheinen **ganz dezente Sonnenstrahlen** schräg herein (Fullscreen-Shader, ≤ 6–8 % Deckkraft oben, nach unten auslaufend, langsam wandernd). Ist alles abgetaut, laufen nur noch die Strahlen mit 15 FPS.
- Wechsel von Schnee zu Regen: liegender Schnee taut über 60 s ab, Regen setzt ein.
- Moduswahl im Tray-Kontextmenü als Untermenü **„Wetter“** mit Radio-Einträgen (über „Einstellungen“) und später zusätzlich im Einstellungsfenster
- Der gewählte Modus wird in `settings.ini` gespeichert und beim Start wiederhergestellt.
- Texte für alle Modi in allen 7 Sprachen
- **Tests:** Parameter je Modus, Übergangs-Interpolation, Abtauen in 60 s (±5 %), Intensitätsfunktion der Sonnenstrahlen, Persistenz
- **Abnahme:** Alle fünf Modi sind umschaltbar, die Übergänge laufen weich, Sonne räumt in etwa einer Minute komplett auf. Die Sonnenstrahlen sind sichtbar, stören aber nicht beim Lesen. Im Sonnenmodus liegt die CPU unter 0,3 %.

## M8: Regen, Tropfen, Lichtbrechung und Bildauswahl
- Regen: schnelle, schräge, halbtransparente Streifen. Bei Regensturm dichter, mit Böen und stärkerer Neigung. Beim Aufprall entstehen kleine Spritzer.
- Tropfen sammeln sich auf Fenster-Oberkanten und auf der Taskleiste: wachsen, verschmelzen, rollen bei zu viel Gewicht über die Kante ab und tropfen herunter. Sie reagieren auf Fensterbewegungen und den Cursor wie der Schnee.
- **Lichtbrechung über ein Hintergrundbild** (keine Bildschirmaufnahme): Jeder Tropfen zeigt einen verkleinerten, umgedrehten und verzerrten Ausschnitt des gewählten Bildes, dazu Glanzlicht, dunkler Rand, Fresnel und leichte Unschärfe. Details im Skill `weather-and-rain`.
- Mitgelieferte Bilder aus `resources/backgrounds/` (vorerst nur `00-demo-meadow.jpg`). Weitere lassen sich später ohne Codeänderung als Datei ergänzen.
- **Einstellungsfenster bekommt Navigation** (`NavigationView`) mit den Seiten **„Darstellung“** und **„Info“**. Auf „Darstellung“:
  - Galerie der mitgelieferten Bilder als Kacheln mit Vorschau und Namen, die Auswahl wirkt sofort
  - Kachel bzw. Button **„Eigenes Bild …“**: Dateiauswahl, Bild wird übernommen, verkleinert und gespeichert. Es erscheint danach als eigene Kachel und lässt sich wieder entfernen.
  - Vorschau: das gewählte Bild mit ein paar Beispiel-Tropfen (statisch gerendert reicht)
- Alle neuen Texte in 7 Sprachen
- **Tests:** Tropfen-Physik (Wachstum, Verschmelzen mit Volumenerhalt, Abrollen), Abbildungsfunktion Tropfen → Bild-UV, Auflösung der Bild-Einstellung, Skalierung eigener Bilder
- **Abnahme:** Regen sieht glaubwürdig aus, die Tropfen zeigen das gewählte Bild sichtbar verkleinert und umgedreht. Bildwechsel und eigenes Bild funktionieren, ungültige Dateien werden abgefangen. Die Effekte erscheinen auf Screenshots. Performance im Budget.

## M9: Komfort und Release
- Tray-Icon gibt es seit M1, die Info-Seite seit M2. Hier nur Robustheit: Explorer-Neustart (`TaskbarCreated`), Theme-Wechsel, DPI-Wechsel
- Pause bei Vollbild-App, gesperrtem Bildschirm und Standby/Resume
- Device-Lost-Behandlung (`DXGI_ERROR_DEVICE_REMOVED`/`RESET`: Device und Swapchains neu erzeugen)
- Neuaufbau bei Monitor- oder DPI-Änderungen
- README mit GIF, Badges und Download-Tabelle
- Release-Workflow (`.github/workflows/release.yml`, bereits vorhanden) mit einem Test-Tag auf einem Fork oder Branch prüfen
- **Abnahme:** Die App läuft 8 h stabil, der Speicher wächst nicht, sie übersteht Monitor-Wechsel und Standby, jede Release-Exe ist < 1 MB. Ein Tag `v0.1.0` erzeugt ein GitHub Release mit drei Portable-ZIPs (App-Ordner inkl. WinAppSDK-Laufzeit, LICENSE, Hinweise, README) und `SHA256SUMS.txt`.

## M10: Microsoft Store (MSIX)
- `packaging/` mit Manifest-Vorlage und Assets, MSIX pro Architektur und `.msixbundle` (Details im Skill `build-and-release`)
- Paket framework-abhängig vom Windows App Runtime (`PackageDependency`). Den Store installiert die Runtime automatisch, das Paket bleibt klein.
- Paket-Erkennung zur Laufzeit: Autostart über `StartupTask` im Paket, über die Registry ohne Paket
- Lokaler Test: selbstsigniertes Zertifikat, Installation per `Add-AppxPackage`. Prüfen, dass Overlay, Click-Through, Tray, Autostart und Settings im Paket funktionieren.
- Der Release-Workflow hängt zusätzlich ein unsigniertes `.msixbundle` für den Upload in Partner Center an.
- **Abnahme:** Das Bundle installiert sich lokal auf x64 (und ARM64, falls ein Gerät vorhanden ist). Das Windows App Certification Kit (`appcert.exe`) läuft ohne Fehler durch.

## Später (optional)
- Einstellungsfenster erweitern (Navigation existiert seit M8): Wettermodus, An/Aus, Sparmodus 30 FPS, Sonnenstrahlen animieren, Mit Windows starten, Persistenz als INI. Außerdem ein manueller Sprachumschalter.
- 3–4 eigene Hintergrundbilder für die Tropfen ergänzen (nur Dateien und Namen, kein Code)
- Weitere „Wetter“: Regen, Blätter, oder Schafe à la eSheep, die auf Fenstern laufen
- Schneemann bauen, wenn genug Schnee auf der Taskleiste liegt 😄
