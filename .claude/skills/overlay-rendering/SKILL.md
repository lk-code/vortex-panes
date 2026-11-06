---
name: overlay-rendering
description: Use when working on src/render or the main loop – click-through overlay windows per monitor, D3D11 + DXGI composition swap chains + DirectComposition, HLSL shaders, instanced flake rendering, snow-cap meshes, frame pacing, pause, device-lost handling.
---

# Overlay Rendering (D3D11 + DirectComposition)

## Overlay-Fenster (eins pro Monitor)
```cpp
DWORD ex = WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TRANSPARENT
         | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
HWND h = CreateWindowExW(ex, kClass, L"", WS_POPUP, x, y, w, h, nullptr, nullptr, hInst, this);
SetLayeredWindowAttributes(h, 0, 255, LWA_ALPHA); // nötig, damit ein Layered-Fenster überhaupt angezeigt wird
ShowWindow(h, SW_SHOWNOACTIVATE);
```
- `WS_EX_LAYERED | WS_EX_TRANSPARENT` ist das, was **prozessübergreifend** Klicks durchlässt. `WM_NCHITTEST → HTTRANSPARENT` allein reicht nicht.
- `WS_EX_NOREDIRECTIONBITMAP` sorgt dafür, dass es keine GDI-Oberfläche gibt. Der Inhalt kommt ausschließlich über DirectComposition. **In M1 verifiziert (2026-11-06):** Click-Through funktioniert mit dieser Kombination. `WindowFromPoint` liefert das darunterliegende Fenster, und ein echter Mausklick aktiviert es.
- Position und Größe kommen aus `MONITORINFO::rcMonitor` (physische Pixel, da PerMonitorV2).
- Topmost geht gelegentlich verloren, etwa durch die Taskleiste oder Startmenü-Animationen. Deshalb alle 2 s `SetWindowPos(h, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE)` aufrufen.
- Die HWNDs der Overlays werden an den `WindowTracker` gemeldet, damit er sie ausfiltert.

## Device und Composition
1. `D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, …, D3D11_CREATE_DEVICE_BGRA_SUPPORT [| DEBUG im Debug-Build], {11_1, 11_0, 10_1, 10_0}, …)`. Schlägt das fehl, kommt **`D3D_DRIVER_TYPE_WARP`** als Fallback (VMs, ARM-Geräte ohne passenden Treiber). Ist der Debug-Layer nicht installiert, wird ohne `DEBUG`-Flag erneut versucht.
2. `IDXGIDevice` → `IDXGIAdapter` → `IDXGIFactory2`
3. `DCompositionCreateDevice(dxgiDevice, IID_PPV_ARGS(&dcomp))`. Ein DComp-Device für alles.
4. Pro Monitor:
   - `CreateSwapChainForComposition` mit `Width/Height` = Monitorgröße, `DXGI_FORMAT_B8G8R8A8_UNORM`, `BufferCount = 2`, `DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL`, `AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED`, `Scaling = DXGI_SCALING_STRETCH`
   - `dcomp->CreateTargetForHwnd(hwnd, TRUE, &target)`, `CreateVisual(&visual)`, `visual->SetContent(swapchain)`, `target->SetRoot(visual)`, `dcomp->Commit()`
   - RTV auf Backbuffer 0. Bei Flip-Model genügt ein RTV, weil `GetBuffer(0)` immer der aktuelle ist. Nach `ResizeBuffers` neu erzeugen.
- Ein `ID3D11DeviceContext` für alle Swapchains. Pro Monitor: RTV setzen, Viewport setzen, Konstantenpuffer mit Monitor-Offset aktualisieren, zeichnen, `Present(0, 0)`.

## Shader (`shaders/`, Build-Zeit via `fxc`, siehe Skill `build-and-release`)
- Shader Model **4.0** (`vs_4_0`/`ps_4_0`), damit auch Feature Level 10_x und WARP funktionieren. Mehr wird nicht benötigt.
- **Konstantenpuffer** pro Frame/Monitor (32 Byte): `float2 originPx` (Monitor-Left/Top), `float2 invSizePx` (1/Breite, 1/Höhe), `float opacity` (globaler Deckkraftfaktor: Einblenden beim Start, später Modusübergänge)
- Pixel → NDC im VS: `ndc = float2((p.x - origin.x) * invSize.x * 2 - 1, 1 - (p.y - origin.y) * invSize.y * 2)`
- **Flocken** (`flake_vs.hlsl`, `flake_ps.hlsl`):
  - Vertexdaten: Quad-Ecken (−1..1) über `SV_VertexID` (0..3, Triangle-Strip). Es gibt keinen Vertexpuffer.
  - Instanzdaten (Input-Slot 0, `D3D11_INPUT_PER_INSTANCE_DATA`, Semantik `FLAKE`): `float2 pos; float radius; float alpha;` (`core/flake_instances.h`: `FlakeInstance`)
  - Kantenglättung **in Pixeleinheiten**: Das Quad reicht `max(radius, 1) + 1` px um das Zentrum (`flakeExtent()` in `core/flake.h`, gleiche Grenze für Culling und Simulation). Der PS rechnet `a = alpha * (1 - smoothstep(inner, outer, dPx))` mit `inner = max(0.6 * r - 0.5, 0)` und `outer = r + 0.5`, die weiche Kante ist also mindestens etwa 1 px breit. Radien unter 1 px werden mit 1 px gezeichnet und die Alpha mit `r²` skaliert. So flackern kleine Flocken bei Sub-Pixel-Bewegung nicht. Ausgabe `float4(a, a, a, a)`, bereits **premultiplied**.
  - Draw: `DrawInstanced(4, visibleCount, 0, 0)`, nur bei `visibleCount > 0`
- **Schneedecken** (`cap_vs.hlsl`, `cap_ps.hlsl`):
  - Vertexdaten: `float2 pos; float4 color;` (premultiplied). Triangle-List, dynamischer Vertexpuffer.
  - Pro sichtbarem Segment und Spalte zwei Dreiecke zwischen Fensterkante und `top - height`. Das Profil wird per Catmull-Rom auf halbe Spaltenbreite geglättet.
  - Schatten: ein 1–2 px hoher Streifen unterhalb der Kante mit `color = (0,0,0,0.15)` premultiplied. Er wird im selben Draw-Call mitgezeichnet, vor dem Schnee.
  - Farbe des Schnees: `#F4F8FF`, oben 1 px heller als Glanzkante (optional)
- **Blend-State:** `SrcBlend = ONE`, `DestBlend = INV_SRC_ALPHA` (für Farbe und Alpha gleich). Kein Depth, kein Culling.

## Puffer
- Instanzpuffer: `D3D11_USAGE_DYNAMIC`, `CPU_ACCESS_WRITE`, Kapazität = maximale Flockenanzahl. Pro Monitor `Map(WRITE_DISCARD)`, nur Flocken mit Bounds-Schnitt hineinschreiben (`collectVisibleFlakes` schreibt direkt in den gemappten Puffer, ohne Zwischenkopie).
- Cap-Vertexpuffer: dynamisch, Kapazität großzügig (z. B. 256k Vertices). Ist sie überschritten, Segmente kappen und im Debug-Log warnen. Niemals pro Frame neu allokieren.

## Main-Loop und Frame-Pacing
```cpp
HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
// nach jedem Frame neu setzen: FramePacer::advance(now), dann SetWaitableTimerEx mit negativer
// relativer Zeit = FramePacer::waitIn100ns(deadline, now, qpcFrequency), Periode 0
// (nicht periodisch: die Periode ist ganzzahlig in ms, 16 ms ergäben 62,5 FPS)
for (;;) {
    DWORD r = MsgWaitForMultipleObjectsEx(paused ? 0 : 1, &timer, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) { if (msg.message == WM_QUIT) return; TranslateMessage(&msg); DispatchMessageW(&msg); }
    if (!paused && r == WAIT_OBJECT_0) frame();   // update + render + Present
}
```
- Zeitmessung über `QueryPerformanceCounter`, Accumulator für feste Sim-Ticks (max. 4 pro Frame, Rest verwerfen)
- Standard: 60 FPS, Sparmodus 30
- **Pause:** einmal Clear auf Transparent + Present für alle Monitore, dann Timer canceln (`CancelWaitableTimer`). Die Loop wartet dann nur auf Nachrichten, das ergibt 0 % CPU.

## Robustheit
- `Present` liefert `DXGI_ERROR_DEVICE_REMOVED` oder `DXGI_ERROR_DEVICE_RESET`: alles unter `render` verwerfen und neu aufbauen. Die Simulation bleibt erhalten.
- `WM_DISPLAYCHANGE`, `WM_DPICHANGED` sowie `WM_SETTINGCHANGE` mit `SPI_SETWORKAREA` und Taskleisten-Änderungen: Monitore neu aufzählen, Overlays neu anlegen.
- `WM_POWERBROADCAST` (`PBT_APMRESUMEAUTOMATIC`): Device prüfen, Zeitbasis zurücksetzen, damit es nach dem Standby keinen großen `dt`-Sprung gibt.
- Bei TDR oder Treiber-Update übernimmt das Device-Lost-Handling.

## Performance-Regeln
- Pro Monitor genau **2 Draw-Calls** (Caps, Flocken) und 2 Map/Unmap-Vorgänge
- Keine `std::vector`-Neuallokation, kein `new`, keine `std::function` und kein `std::string` im Frame
- Debug-Layer (`D3D11_CREATE_DEVICE_DEBUG`) nur im Debug-Build, im Release niemals
- Messen: Task-Manager (CPU, GPU), PIX oder GPUView bei Bedarf. Die Frame-Zeit der CPU per QPC im Debug loggen (Ziel < 0,3 ms).
