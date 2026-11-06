"""Generates the built-in demo background for rain-drop refraction (no external artwork, no license issues).

Usage:  python tools/make_demo_background.py      (requires Pillow)
Output: resources/backgrounds/00-demo-meadow.jpg
"""
import math
import random
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "resources" / "backgrounds" / "00-demo-meadow.jpg"
W, H = 1600, 1000
rnd = random.Random(42)


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def main():
    img = Image.new("RGB", (W, H))
    d = ImageDraw.Draw(img)
    # sky: deep blue -> light blue -> warm near horizon
    for y in range(H):
        t = y / (H * 0.55)
        if t < 1:
            c = lerp((52, 120, 206), (150, 205, 245), t / 0.7) if t < 0.7 else lerp((150, 205, 245), (255, 226, 170), (t - 0.7) / 0.3)
        else:
            c = (255, 226, 170)
        d.line([(0, y), (W, y)], fill=c)
    # sun glow
    glow = Image.new("L", (W, H), 0)
    gd = ImageDraw.Draw(glow)
    sx, sy = int(W * 0.72), int(H * 0.30)
    for r in range(260, 0, -4):
        gd.ellipse([sx - r, sy - r, sx + r, sy + r], fill=int(255 * (1 - r / 260) ** 2))
    img.paste(Image.new("RGB", (W, H), (255, 244, 210)), (0, 0), glow)
    # soft clouds
    clouds = Image.new("L", (W, H), 0)
    cd = ImageDraw.Draw(clouds)
    for _ in range(26):
        cx, cy = rnd.randint(0, W), rnd.randint(40, int(H * 0.35))
        for _ in range(6):
            rx, ry = rnd.randint(50, 140), rnd.randint(20, 50)
            ox, oy = rnd.randint(-90, 90), rnd.randint(-20, 20)
            cd.ellipse([cx + ox - rx, cy + oy - ry, cx + ox + rx, cy + oy + ry], fill=150)
    img.paste(Image.new("RGB", (W, H), (255, 255, 255)), (0, 0), clouds.filter(ImageFilter.GaussianBlur(18)))

    # layered hills (far -> near)
    layers = [((120, 160, 190), 0.52, 40, 0.004), ((84, 150, 110), 0.60, 55, 0.006),
              ((58, 132, 70), 0.70, 60, 0.005), ((40, 110, 50), 0.82, 45, 0.007)]
    for color, base, amp, freq in layers:
        phase = rnd.random() * 10
        pts = [(0, H)]
        for x in range(0, W + 10, 10):
            y = H * base - amp * math.sin(x * freq + phase) - amp * 0.5 * math.sin(x * freq * 2.7 + phase * 1.3)
            pts.append((x, y))
        pts.append((W, H))
        d.polygon(pts, fill=color)
    # tree silhouettes on the middle hill
    for _ in range(40):
        x = rnd.randint(0, W)
        y = H * 0.70 - 60 * math.sin(x * 0.005) + rnd.randint(-10, 30)
        h = rnd.randint(40, 110)
        w = h * 0.38
        d.polygon([(x, y - h), (x - w, y), (x + w, y)], fill=(30, 85, 45))
    # flower meadow in the foreground
    palette = [(250, 90, 110), (255, 210, 60), (255, 255, 255), (170, 110, 230), (255, 150, 60)]
    for _ in range(1600):
        x = rnd.randint(0, W)
        y = rnd.randint(int(H * 0.80), H)
        r = 2 + (y - H * 0.8) / (H * 0.2) * 7 * rnd.random()
        d.ellipse([x - r, y - r, x + r, y + r], fill=rnd.choice(palette))
    img = img.filter(ImageFilter.GaussianBlur(1.2))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    img.save(OUT, "JPEG", quality=85, optimize=True, progressive=True)
    print("written", OUT, OUT.stat().st_size // 1024, "KB")


if __name__ == "__main__":
    main()
