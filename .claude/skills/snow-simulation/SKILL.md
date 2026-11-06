---
name: snow-simulation
description: Use when working on src/core – falling flakes, wind, snow caps/height maps, sliding, melting, detecting window move/fling/resize/vanish events, and cursor brushing.
---

# Snow Simulation

Alles hier liegt in `src/core`, ist plattformunabhängiges C++20 ohne `<windows.h>`, mit GoogleTest testbar und arbeitet in physischen Pixeln.

## Zeitschritt
- Der Simulationstakt ist fest: `dt = 1/60 s`. Pro Render-Frame laufen per Accumulator 0–4 Ticks. Ist der Rückstand größer, wird er verworfen (kein „Spiral of Death“).
- Zufall über einen eigenen RNG (PCG32) mit Seed, der als Referenz übergeben wird, damit Tests deterministisch sind.
- Datenlayout cachefreundlich: Flocken in einem zusammenhängenden `std::vector<Flake>`, Iteration ohne virtuelle Aufrufe. Der Compiler soll die Integrationsschleife autovektorisieren können. Falls nötig, SoA-Layout nach Rückfrage.

## Fallende Flocken
- `struct Flake { float x, y, vx, vy, radius, phase, depth, alpha; };` mit `depth` ∈ [0,1].
- Für Tiefenwirkung bestimmt `Depth` den Radius (1–4 px), die Fallgeschwindigkeit (40–120 px/s), die Alpha (0.4–1.0) und die Windstärke.
- Bewegung pro Tick: `Vy` nähert sich der Endgeschwindigkeit an. Für `X` gilt `Wind(t) * Depth + sin(Phase) * Wobble` mit `Phase += dt * freq`.
- Wind: Summe aus zwei langsamen Sinuskurven oder 1D-Value-Noise. Sanft, keine Sprünge.
- Spawn: Gleichmäßig über die Breite des virtuellen Bildschirms, knapp oberhalb des jeweils oberen Monitorrands. Die Rate kommt aus `Settings.Intensity` (z. B. 30/80/200 Flocken pro Sekunde und Megapixel-Breite).
- Kapazität: harte Obergrenze im Pool (z. B. 4000). Ist er voll, wird nicht gespawnt.
- **Nur Flocken mit `Depth > 0.5` kollidieren** mit Flächen. Die „hinteren“ fallen durch, das wirkt räumlicher und spart Arbeit.

## Schneedecke (SnowCap)
- Pro Fläche ein `std::vector<float> heights`, Länge `ceil(width / ColumnWidth)`. Die Werte sind relativ zur linken Kante, damit der Schnee beim Verschieben einfach mitwandert.
- **Ablagern:** Eine Flocke trifft, wenn sie im Tick die Linie `top - Heights[col]` von oben überquert **und** `x` in einem sichtbaren Segment liegt. Dann gilt `Heights[col] += FlakeVolume(radius)`, und die Flocke wird entfernt.
- **Abrutschen (Schüttwinkel):** Jeden Tick (oder jeden 2. Tick) wird über alle Spalten iteriert. Ist `h[i] - h[i±1] > MaxSlope`, wird die Hälfte der Differenz minus `MaxSlope` zum Nachbarn verschoben. Die Iterationsrichtung wechselt pro Tick, sonst entsteht eine Drift.
- **Rand:** Läuft an Spalte 0 oder n−1 Material über die Kante, wird es als neue Partikel mit kleiner seitlicher Geschwindigkeit ausgegeben.
- **Maximum:** `MaxSnowHeight` (Standard 14 px). Darüber wird der Überschuss abgerutscht oder verworfen.
- **Schmelzen:** `h -= MeltRate * dt` mit leichtem Rauschen pro Spalte. Nie unter 0.
- Die Decke wird **nur in sichtbaren Segmenten** befüllt und gezeichnet. Verdeckte Teile behalten ihren Wert.

## Ereigniserkennung (Snapshot-Diff)
Pro Handle werden der vorherige und der aktuelle Snapshot verglichen:
- **Verschwunden** (nicht mehr im Snapshot): Die Decke fällt als Partikel ab (siehe unten), danach wird die Cap entfernt.
- **Größe geändert** (`|Δw| > 2 || |Δh| > 2`): Die Decke fällt ab.
- **Bewegt:** Die Fenstergeschwindigkeit `v = Δpos / dt` wird per EMA (α≈0.4) geglättet.
  - `|v| > FlingThreshold` (Standard 1500 px/s): Die Decke fliegt mit `v * 0.8` plus Streuung ab.
  - sonst: Die Decke bleibt liegen. Sie ist relativ gespeichert und wandert damit automatisch mit.
- Bei einem Sprung zwischen Monitoren mit anderer DPI (Größe ändert sich) greift die Regel „Größe geändert“. Das ist gewollt.

## Decke zu Partikeln
- Pro Spalte `n = ceil(h / FlakeVolume)` Partikel, max. 3 pro Spalte, Position zufällig im Bereich `[top - h, top]`.
- Die Startgeschwindigkeit ist das übergebene `v` plus zufällige Streuung. Diese Partikel sind „Klumpen“: Sie fallen schneller und kollidieren **nicht** sofort wieder mit derselben Fläche (Ignore-Handle für 0,5 s).

## Cursor-Brush
- Eingabe: Cursorposition und geglättete Geschwindigkeit.
- Ist die Geschwindigkeit größer als ein Mindestwert (z. B. 200 px/s), wird für jede Cap geprüft, welche Spalten im `BrushRadius` (Standard 18 px) liegen und deren Schneeoberfläche den Cursor berührt.
- Abgetragen wird mit Gauß-Abfall zum Rand hin. Das Material wird zu Partikeln mit `cursorVelocity * 0.5` plus Aufwärtsimpuls.

## Tests (Pflicht)
- Ablagern erhöht genau eine Spalte, Flocke wird entfernt
- Abrutschen erhält die Gesamtmasse (bis auf den Randüberlauf)
- Maximum und Schmelzen werden eingehalten
- Diff-Erkennung: bewegt langsam / Fling / Resize / verschwunden, jeweils mit erwarteter Partikelanzahl
- Brush trägt nur Spalten im Radius ab
- Determinismus: gleicher Seed und gleiche Eingaben ergeben den gleichen Zustand
