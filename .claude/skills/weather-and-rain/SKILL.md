---
name: weather-and-rain
description: Use when working on weather modes (light snow, snowstorm, sun, light rain, rainstorm), mode transitions, melting/evaporation, sun rays, rain streaks, water droplets collecting on window edges and the taskbar, the droplet refraction effect and the selectable refraction background image (built-in images + custom image via WIC).
---

# Wettermodi, Sonne und Regen

## Modi und Parameter
Ein `WeatherPreset` (in `core`, reine Daten) pro Modus. Die aktiven Parameter werden bei einem Moduswechsel über **3–5 s** interpoliert (Smoothstep), es gibt keinen harten Schnitt.

| Modus | Partikel | Rate | Wind | Besonderheit |
|---|---|---|---|---|
| **Leichter Schnee** (Standard) | Flocken | niedrig | sanft | wie M3–M6 |
| **Schneesturm** | Flocken, kleiner und schneller | ca. 5× | stark, böig (Böen alle 2–6 s), Neigung bis 40° | Decken wachsen schneller. Starke Böen wehen etwas Schnee von den Kanten. |
| **Sonne** | keine | 0 | – | Abtauen bzw. Verdunsten in 60 s, Sonnenstrahlen |
| **Leichter Regen** | Regenstreifen | mittel | leicht | Tropfen sammeln sich |
| **Regensturm** | Regenstreifen, länger und dichter | ca. 4× | stark, böig, Neigung bis 35° | Tropfen wachsen schnell und rollen öfter ab |

- Die bisherige Einstellung „Intensität“ entfällt, sie steckt jetzt in den Modi.
- Schnee bei Regen oder Sonne: Liegender Schnee taut **linear über 60 s** ab (`h -= h0 / 60 * dt`, `h0` = Höhe beim Wechsel). Kleine Mengen bröckeln nicht als Partikel ab, sie schmelzen.
- Tropfen bei Schnee oder Sonne: Sie schrumpfen (verdunsten) ebenfalls über 60 s.
- **Sonne, wenn alles weg ist:** Simulation stoppen. Nur noch die Sonnenstrahlen laufen mit reduzierter Bildrate (siehe unten).

## Sonnenstrahlen (Modus Sonne)
- **Ganz dezente** Lichtstrahlen, die vom **oberen Rand jedes Monitors** schräg nach unten durchscheinen, wie Sonnenlicht durch ein Fenster oder Baumkronen. Sie dürfen die Lesbarkeit nie beeinträchtigen.
- Umsetzung als **Fullscreen-Pixel-Shader**, ohne Textur und ohne Desktop-Aufnahme:
  - 5–8 Strahlenbänder pro Monitor, die von einem gemeinsamen virtuellen Lichtpunkt oberhalb des Monitors ausgehen (leicht seitlich versetzt, ca. 15–25° Neigung). Breite, Abstand und Helligkeit variieren per 1D-Noise.
  - Die Intensität fällt mit dem Abstand vom oberen Rand ab (z. B. `exp(-y / (0.45 * height))`). Unten auf dem Monitor ist praktisch nichts mehr zu sehen.
  - Farbe warm (≈ `#FFF2CC`). Maximale Deckkraft **≤ 6–8 %** direkt am oberen Rand, premultiplied und weich ausgeblendet, keine harten Kanten.
  - Animation: sehr langsames Wandern und Pulsieren (Periode 20–40 s). Dazu ganz optional ein paar winzige, langsam schwebende Staubpartikel im Licht (≤ 30 pro Monitor, Alpha ≤ 15 %).
- Ein- und Ausblenden über die normale Modus-Überblendung (3–5 s)
- **Performance:** Sobald kein Schnee und keine Tropfen mehr da sind, wird die Bildrate auf **15 FPS** gesenkt (Ziel < 0,3 % CPU). Die Strahlen sind ein einzelner Draw-Call. Eine Option „Sonnenstrahlen animieren“ (Standard an) kommt später in die Einstellungen. Ist sie aus, wird ein statisches Bild einmal gerendert, danach liegt die CPU bei 0 %.
- Bei Vollbild-Apps wird wie immer pausiert.

## Regen (fallend)
- Partikel sind **Streifen**: Position, Geschwindigkeit (400–1100 px/s), Länge proportional zur Geschwindigkeit, Breite 1–2 px, Alpha 0.15–0.35. Gerendert als instanced Quads entlang des Geschwindigkeitsvektors mit weichem Verlauf zum Ende hin.
- Tiefenstufen wie beim Schnee. Nur vordere Streifen treffen Kanten.
- Beim Auftreffen auf eine Kante entsteht ein kleiner Spritzer (2–4 Mini-Partikel, Lebensdauer < 0,3 s), und der Kante wird Wasservolumen zugeführt (siehe Tropfen).
- Kein eigener Brechungseffekt für die fallenden Streifen, das kostet zu viel und fällt nicht auf.

## Tropfen auf Kanten
- Pro Fläche (wie `SnowCap`) eine Liste von Tropfen: `x` (relativ zur linken Kante), `volume`, daraus Radius und Höhe (abgeflachte Halbkugel, Kontaktwinkel ca. 60–70°). Sie sitzen **auf** der Oberkante.
- **Wachstum:** Auftreffendes Wasser landet beim nächsten Tropfen in Reichweite oder bildet einen neuen kleinen Tropfen.
- **Verschmelzen:** Überlappen sich zwei Tropfen, entsteht einer mit der Summe der Volumina am gewichteten Mittelpunkt. Das Volumen bleibt erhalten (Test!).
- **Abrollen und Abtropfen:** Ab einem Grenzvolumen oder an der Fensterkante rollt der Tropfen über die Ecke ab und fällt als fallender Tropfen herunter (Schwerkraft, kleine Streckung in Fallrichtung). Er trifft gegebenenfalls eine tiefere Kante und vereinigt sich dort.
- **Fensterbewegung, Resize, Minimieren, Cursor:** Es gelten dieselben Regeln wie beim Schnee. Tropfen fallen bzw. fliegen ab, und der Cursor wischt sie weg und erzeugt dabei kleine Spritzer.
- Obergrenze an Tropfen pro Fläche und global (z. B. 2000), die kleinsten verdunsten zuerst.

## Lichtbrechung über ein Hintergrundbild
**Es gibt keine Bildschirmaufnahme.** Ein echter Tropfen auf einer Scheibe zeigt die Welt *draußen*, verkleinert und auf den Kopf gestellt. Diese „Welt draußen“ ist ein wählbares **Hintergrundbild**. Dadurch bleiben die Effekte auf Screenshots, in OBS und in Teams sichtbar, es entstehen keine Datenschutzfragen, und es funktioniert auch über Remote Desktop.

### Bildquellen
- **Mitgelieferte Bilder:** alle Dateien in `resources/backgrounds/*.jpg`. Der Dateiname gibt Reihenfolge und ID vor, z. B. `00-demo-meadow.jpg` → ID `demo-meadow`. CMake sammelt die Dateien per `file(GLOB … CONFIGURE_DEPENDS)`, bettet sie als `RCDATA` ein und erzeugt daraus eine Tabelle (ID, Ressourcen-ID, String-ID für den Anzeigenamen). Weitere Bilder kommen später einfach als Datei dazu, plus ein Anzeigename in den 7 Sprachdateien (`IDS_BG_<ID>`). Fehlt der Name, wird die ID angezeigt. Ein Check in `tools/Check-Strings.ps1` meldet das als Warnung.
  - Vorgaben für Bilder: JPEG, ca. 1600×1000, ≤ 400 KB, keine Personen, kein Text, eigene Bilder oder lizenzfrei mit dokumentierter Lizenz (siehe Lizenzregeln in `CLAUDE.md`)
  - `00-demo-meadow.jpg` ist ein Demo-Bild, erzeugt durch `tools/make_demo_background.py`. Es ist ohne fremde Vorlage entstanden und hat deshalb keine Lizenzfragen. Es bleibt Standard, bis eigene Bilder da sind.
- **Eigenes Bild:** Der Nutzer wählt eine Datei aus (siehe Einstellungen). Unterstützt wird alles, was **WIC** dekodieren kann: JPEG, PNG, BMP, GIF (erstes Bild), TIFF, dazu WebP und HEIC, falls die Windows-Codecs installiert sind. WIC ist Teil von Windows, es braucht keine Bibliothek.
  - Beim Übernehmen wird das Bild per WIC dekodiert, auf maximal 1920 px lange Kante skaliert und als JPEG nach `%AppData%\VortexPanes\backgrounds\custom.jpg` gespeichert. Das Original darf danach verschoben oder gelöscht werden.
  - Fehler (kein Bild, defekt, zu groß > 50 MB) führen zu einer verständlichen Meldung in 7 Sprachen, und die alte Auswahl bleibt erhalten.
- **Fallback:** Lässt sich kein Bild laden, wird im Shader ein prozeduraler Himmel-und-Wiese-Verlauf erzeugt. Die Tropfen sehen nie kaputt aus.
- Einstellung in `settings.ini`: `RainBackground=builtin:demo-meadow` bzw. `RainBackground=custom`

### Textur
- Das Bild wird beim Start (bzw. bei Änderung) einmal per WIC geladen, als `DXGI_FORMAT_B8G8R8A8_UNORM_SRGB`-Textur mit **vollständiger Mip-Kette** hochgeladen (`GenerateMips`) und bleibt auf der GPU. Danach entstehen keine Kosten pro Frame.
- Geladen wird nur, wenn ein Regenmodus aktiv ist oder Tropfen existieren. Sonst wird die Textur freigegeben.

### Shader
- Pro Tropfen ein Quad (instanced). Im Pixel-Shader gilt:
  - Lokale Koordinate im Tropfen. Daraus wird die Höhe der abgeflachten Kugel und die **Normale** berechnet.
  - **Abbildung der Welt draußen:** Die Bildschirmposition des Tropfens wird auf das Bild abgebildet („cover“ über den jeweiligen Monitor). Ausgehend davon wird ein **verkleinerter, auf den Kopf gestellter Ausschnitt** gesampelt (`uv = center - local * lensScale`, mit negativem Vorzeichen für die Umkehrung) und mit der Normalen zusätzlich verzerrt.
  - **Unschärfe:** Gesampelt wird mit einem Mip-Bias (Tiefenschärfe-Look), kleine Tropfen etwas unschärfer.
  - Optional eine leichte chromatische Aberration (R, G, B mit minimal verschiedenem Offset)
  - **Fresnel:** Zum Rand hin mehr Reflexion bzw. Abdunklung, ein dünner dunkler Randring für die Kontur
  - **Glanzlicht:** ein kleines weiches Highlight oben links (Lichtrichtung fix), dazu ein schwacher Kaustik-Aufheller unten im Tropfen
  - Dezente Tönung (Wasser leicht kühl), Alpha am Rand weich ausblenden
- Das Ergebnis wird premultiplied in die Overlay-Swapchain geschrieben, wie alles andere.
- Die Abbildungsfunktion (Bildschirmposition und lokale Koordinate → UV) gibt es zusätzlich als CPU-Referenz in `core`, damit sie testbar ist.

## Moduswahl (UI)
- Tray-Kontextmenü: Untermenü **„Wetter“** mit 5 Radio-Einträgen (`MFT_RADIOCHECK`) über „Einstellungen“. Die Wahl wirkt sofort und wird in `settings.ini` gespeichert.
- Später zusätzlich im Einstellungsfenster (WinUI), z. B. als `RadioButtons` mit Symbolen.
- Die Auswahl des Hintergrundbilds liegt im Einstellungsfenster (siehe Skill `winui-settings`, Seite „Darstellung“).
- Texte in allen 7 Sprachen (STRINGTABLE).

## Tests (Pflicht)
- Presets: Parameter je Modus, Interpolation zwischen zwei Modi (Anfang, Mitte, Ende)
- Abtauen und Verdunsten: nach 60 s ist alles weg (±5 %), monoton fallend
- Tropfen: Wachstum, Verschmelzen mit Volumenerhalt, Abrollen ab Grenzvolumen, Ober- und Untergrenzen
- Sonnenstrahlen: Intensitätsfunktion (oben maximal ≤ 0.08, unten ≈ 0) als CPU-Referenz
- Abbildungsfunktion Tropfen → Bild-UV inkl. Umkehrung (CPU-Referenz)
- Bildquellen: Auflösung der Einstellung (`builtin:<id>`, `custom`, unbekannte ID → Standard), Skalierung auf max. 1920 px
