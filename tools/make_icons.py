"""Generates all VortexPanes icons from code (no external artwork).

Usage:  python tools/make_icons.py      (requires Pillow)
Output: resources/icons/*.png, resources/app.ico, resources/tray_light.ico, resources/tray_dark.ico
"""
import math
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "resources"
SS = 4  # supersampling factor
ICO_SIZES = [16, 20, 24, 32, 40, 48, 64, 96, 128, 256]
TRAY_SIZES = [16, 20, 24, 32, 40, 48, 64]


def snowflake(draw, cx, cy, r, width, color, detail=True):
    """Six arms, each with two pairs of side branches (fewer for tiny sizes)."""
    for i in range(6):
        a = math.radians(90 + i * 60)
        ex, ey = cx + r * math.cos(a), cy - r * math.sin(a)
        draw.line([(cx, cy), (ex, ey)], fill=color, width=width)
        draw.ellipse([ex - width / 2, ey - width / 2, ex + width / 2, ey + width / 2], fill=color)
        branches = [(0.55, 0.32)] if not detail else [(0.42, 0.26), (0.72, 0.22)]
        for pos, length in branches:
            bx, by = cx + r * pos * math.cos(a), cy - r * pos * math.sin(a)
            for side in (-1, 1):
                b = a + side * math.radians(45)
                tx, ty = bx + r * length * math.cos(b), by - r * length * math.sin(b)
                draw.line([(bx, by), (tx, ty)], fill=color, width=width)
                draw.ellipse([tx - width / 2, ty - width / 2, tx + width / 2, ty + width / 2], fill=color)
    hub = width * 1.1
    draw.ellipse([cx - hub, cy - hub, cx + hub, cy + hub], fill=color)


def app_icon(size):
    s = size * SS
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    # vertical gradient, deep night blue -> ice blue
    grad = Image.new("RGBA", (s, s))
    top, bottom = (24, 58, 128), (86, 168, 236)
    gd = ImageDraw.Draw(grad)
    for y in range(s):
        t = y / (s - 1)
        gd.line([(0, y), (s, y)], fill=tuple(int(top[k] + (bottom[k] - top[k]) * t) for k in range(3)) + (255,))
    mask = Image.new("L", (s, s), 0)
    pad = s * 0.04
    ImageDraw.Draw(mask).rounded_rectangle([pad, pad, s - pad, s - pad], radius=s * 0.22, fill=255)
    img.paste(grad, (0, 0), mask)
    d = ImageDraw.Draw(img)
    # snow cap along the top edge, like snow lying on a window (only where it stays readable)
    if size >= 32:
        def cap_points(offset):
            pts = [(0, 0), (s, 0)]
            for i in range(41):
                x = s - s * i / 40
                y = s * (0.15 + 0.04 * math.sin(i / 40 * math.pi * 3.2 + 0.6)) + offset
                pts.append((x, y))
            return pts
        cap = Image.new("RGBA", (s, s), (0, 0, 0, 0))
        cd = ImageDraw.Draw(cap)
        cd.polygon(cap_points(s * 0.03), fill=(10, 30, 80, 110))   # soft shadow under the snow
        cd.polygon(cap_points(0), fill=(244, 248, 255, 255))        # the snow itself
        cap_mask = Image.new("L", (s, s), 0)
        cap_mask.paste(cap.split()[3], (0, 0), mask)
        img.paste(cap, (0, 0), cap_mask)
    detail = size >= 32
    snowflake(d, s / 2, s * 0.53, s * 0.30, max(SS, int(s * (0.055 if detail else 0.08))), (255, 255, 255, 255), detail)
    return img.resize((size, size), Image.LANCZOS)


def tray_icon(size, color):
    s = size * SS
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    detail = size >= 32
    snowflake(d, s / 2, s / 2, s * 0.44, max(SS, int(s * (0.08 if detail else 0.11))), color, detail)
    return img.resize((size, size), Image.LANCZOS)


def save_ico(path, images):
    images[-1].save(path, format="ICO", sizes=[im.size for im in images], append_images=images[:-1])


def main():
    (OUT / "icons").mkdir(parents=True, exist_ok=True)
    app = [app_icon(n) for n in ICO_SIZES]
    save_ico(OUT / "app.ico", app)
    app_icon(1024).save(OUT / "icons" / "app_1024.png")
    # tray: light glyph for dark taskbar, dark glyph for light taskbar
    save_ico(OUT / "tray_dark.ico", [tray_icon(n, (255, 255, 255, 255)) for n in TRAY_SIZES])
    save_ico(OUT / "tray_light.ico", [tray_icon(n, (32, 32, 32, 255)) for n in TRAY_SIZES])
    print("icons written to", OUT)


if __name__ == "__main__":
    main()
