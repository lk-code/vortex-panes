---
name: app-shell
description: Use when working on the app shell in src/app – startup without a taskbar window, single instance, the notification-area (tray) icon and its context menu, tray light/dark icon, icons, resources (.rc, STRINGTABLE, VERSIONINFO) and localization.
---

# App-Shell (Tray, Ressourcen, Lokalisierung)

## Start
1. Single-Instance: `CreateMutexW(nullptr, TRUE, L"Local\\VortexPanes.SingleInstance")`. Bei `ERROR_ALREADY_EXISTS` wird der laufenden Instanz eine registrierte Nachricht (`RegisterWindowMessageW(L"VortexPanes.ShowSettings")`) per `PostMessage(HWND_BROADCAST …)` geschickt, damit sie die Einstellungen öffnet. Danach beendet sich die neue Instanz.
2. Ein **Message-Only-Fenster** wird angelegt (`CreateWindowExW(0, kClass, L"", 0, 0,0,0,0, HWND_MESSAGE, …)`). Es ist unsichtbar, hat keinen Taskleisten-Eintrag und ist der Besitzer des Tray-Icons.
   - Achtung: `HWND_BROADCAST` erreicht keine Message-Only-Fenster. Für die Single-Instance-Nachricht deshalb das Fenster per `FindWindowExW(HWND_MESSAGE, nullptr, kClass, nullptr)` suchen und direkt `PostMessageW` aufrufen.
3. Tray-Icon hinzufügen, danach die Overlays anlegen. **Nie** `ShowWindow` auf ein normales Top-Level-Fenster ohne `WS_EX_TOOLWINDOW`, solange die Einstellungen nicht offen sind.

## Tray-Icon
- `NOTIFYICONDATAW` mit `cbSize = sizeof(...)`, `hWnd` = Message-Only-Fenster, `uID = 1`, `uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP`, `uCallbackMessage = WM_APP + 1`, Tooltip aus der STRINGTABLE („VortexPanes“).
- **Keine `guidItem`/`NIF_GUID`.** Die GUID ist an den Exe-Pfad gebunden, bei Portable-ZIPs ändert sich der Pfad, und das Icon würde dann nicht mehr erscheinen.
- `NIM_ADD`, dann `NIM_SETVERSION` mit `uVersion = NOTIFYICON_VERSION_4`. Danach gilt: `LOWORD(lParam)` ist das Ereignis, `GET_X_LPARAM(wParam)`/`GET_Y_LPARAM(wParam)` ist die Ankerposition.
- Ereignisse:
  - `WM_CONTEXTMENU` (Rechtsklick oder Tastatur): Kontextmenü an der Ankerposition
  - `NIN_SELECT` / `NIN_KEYSELECT` (Linksklick oder Enter) sowie `WM_LBUTTONDBLCLK`: Einstellungen öffnen
- Kontextmenü: `CreatePopupMenu` mit den Einträgen **Einstellungen** (`IDM_SETTINGS`, `MF_DEFAULT`, also fett) und **Beenden** (`IDM_EXIT`), getrennt durch `MF_SEPARATOR`. Vor `TrackPopupMenuEx(…, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN …)` immer `SetForegroundWindow(hwnd)`, danach `PostMessageW(hwnd, WM_NULL, 0, 0)`. Sonst schließt das Menü nicht, wenn man daneben klickt. Das Menü so aufbauen, dass sich später leicht Einträge ergänzen lassen (Tabelle aus ID, String-ID und Handler).
- **Beenden:** `NIM_DELETE`, Overlays und Einstellungsfenster zerstören, `PostQuitMessage(0)`
- **Explorer-Neustart:** `RegisterWindowMessageW(L"TaskbarCreated")`. Kommt diese Nachricht, wird das Icon erneut per `NIM_ADD` + `NIM_SETVERSION` hinzugefügt.
- **Icon nach Theme:** Der Registry-Wert `HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\SystemUsesLightTheme` bestimmt die Variante. `1` bedeutet helle Taskleiste, dann `tray_light.ico` (dunkle Glyphe). `0` bedeutet dunkle Taskleiste, dann `tray_dark.ico` (helle Glyphe). Bei `WM_SETTINGCHANGE` mit `lParam == L"ImmersiveColorSet"` prüfen und per `NIM_MODIFY` tauschen.
- **DPI:** Icons über `LoadIconMetric(hInst, MAKEINTRESOURCEW(id), LIM_SMALL, &hIcon)` laden, nicht über `LoadIcon`. Bei DPI-Wechsel neu laden.

## Einstellungsfenster
Das Fenster wird mit **WinUI 3 über XAML Islands** umgesetzt, siehe Skill `winui-settings`. Die App-Shell ruft nur `SettingsWindow::show()` auf. Ist das Fenster schon offen, wird es in den Vordergrund geholt.

## Ressourcen und Lokalisierung
- `app.rc.in` → `configure_file` → `app.rc`. Inhalt:
  - `IDI_APP ICON "app.ico"` als **erstes** Icon, damit es das Exe-Icon im Explorer wird. Dazu `IDI_TRAY_LIGHT`, `IDI_TRAY_DARK`.
  - `VERSIONINFO`: FileVersion und ProductVersion aus `PROJECT_VERSION`, CompanyName = `VP_DEVELOPER`, ProductName/FileDescription = `VP_APP_NAME`, LegalCopyright „© 2026 lk-code, MIT License“
  - Das Einstellungsfenster (WinUI) hat **ein** XAML-Layout ohne Texte. Alle Texte setzt der Code zur Laufzeit aus der STRINGTABLE, so gibt es eine einzige Übersetzungsquelle für Tray und Einstellungen. Keine `.resw`-Dateien.
  - `THIRD_PARTY_NOTICES` als `RCDATA`
  - `#include` der Sprachdateien aus `resources/strings/`
- **Sprachdateien:** je Sprache eine Datei `resources/strings/<tag>.rc` mit genau einem `STRINGTABLE`-Block und passendem `LANGUAGE`:
  | Datei | LANGUAGE |
  |---|---|
  | `en-US.rc` | `LANG_ENGLISH, SUBLANG_ENGLISH_US` (Referenz, Fallback) |
  | `de-DE.rc` | `LANG_GERMAN, SUBLANG_GERMAN` |
  | `es-ES.rc` | `LANG_SPANISH, SUBLANG_SPANISH_MODERN` |
  | `fr-FR.rc` | `LANG_FRENCH, SUBLANG_FRENCH` |
  | `zh-CN.rc` | `LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED` |
  | `ja-JP.rc` | `LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN` |
  | `ko-KR.rc` | `LANG_KOREAN, SUBLANG_KOREAN` |
- **Encoding:** Alle `.rc`-Dateien sind UTF-8 **mit** `#pragma code_page(65001)` in der ersten Zeile, und `rc.exe` bekommt `/c65001` (CMake: `CMAKE_RC_FLAGS`). Ohne das werden chinesische, japanische und koreanische Zeichen zerstört. Bei CJK-Texten in M1 im laufenden Programm prüfen, dass nichts als `?` oder `□` erscheint.
- **Sprachwahl:** Windows lädt automatisch die Variante der Anzeigesprache, sonst `en-US`. Zur Sicherheit wird beim Start einmal `SetThreadPreferredUILanguages` mit der Benutzersprache und `en-US` als Fallback gesetzt, damit z. B. `de-AT` sauber auf Deutsch fällt (gleiche Primärsprache). Ein manueller Sprachumschalter kommt später in die Einstellungen.
- **Schriften:** Nur Systemschriften. Segoe UI hat kein CJK, Windows ergänzt die Zeichen per Font-Linking (Microsoft YaHei, Yu Gothic UI, Malgun Gothic) automatisch. Für eigene Schriften `CreateFontIndirectW` mit `lfFaceName = L"Segoe UI"`, keine festen CJK-Fonts. Bei DirectWrite übernimmt der System-Font-Fallback.
- **Layout:** Texte sind je nach Sprache unterschiedlich lang (Französisch und Spanisch bis ca. 40 % länger als Englisch). Controls deshalb breit genug anlegen oder dynamisch per `GetTextExtentPoint32W` bzw. DirectWrite messen. Nichts darf abgeschnitten werden.
- **Übersetzungen:** Claude Code erstellt sie selbst. Jede Datei bekommt einen Kopfkommentar `// Machine-translated – please review` (außer en-US und de-DE). Kurze, neutrale UI-Sprache im Stil der Windows-Einstellungen (Deutsch: „Einstellungen“, „Beenden“, Sie-Form vermeiden, neutral formulieren).
- **Vollständigkeit:** `tools/Check-Strings.ps1` vergleicht alle Sprachdateien mit `en-US.rc` (gleiche IDs, keine leeren Strings, gleiche Platzhalter wie `%s` oder `{0}`). Das Skript läuft in der CI als eigener Schritt (PowerShell ist auf den Windows-Runnern vorhanden) und schlägt bei Abweichungen fehl.
- Store (M10): Das Paket-Manifest bekommt `<Resources>` für alle 7 Sprachen. Das Store-Listing kann später in denselben Sprachen gepflegt werden.
- Generierter Header `app_info.h` (via `configure_file`) mit `kAppName`, `kDeveloper`, `kWebsite`, `kVersion` als `constexpr const wchar_t*`.

## Tests
- Reine Logik testen: Sprach-Fallback (`de-AT` → Deutsch, `pt-BR` → Englisch), Theme-Auswertung (Registry-Wert → Icon-Variante) als Funktion mit injiziertem Wert, Architektur-String, Aufbau der Menütabelle.
- Manuell (in der Meilenstein-Zusammenfassung auflisten):
  - Tray-Icon bei heller und dunkler Taskleiste
  - Explorer neu starten: Das Icon kommt zurück
  - Zweite Instanz starten: Das Einstellungsfenster der ersten öffnet sich
  - Link öffnet den Browser
  - DPI 100 %, 150 % und 200 %
  - alle 7 Sprachen über die Windows-Anzeigesprache (oder per Test-Schalter `--lang ja-JP` im Debug-Build, der `SetThreadUILanguage` setzt). Dabei prüfen, dass CJK-Zeichen korrekt erscheinen und nichts abgeschnitten wird.
