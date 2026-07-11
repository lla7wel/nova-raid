#!/usr/bin/env python3
"""Generate the repository banner and social-preview image.

Renders the NOVA RAID wordmark with the game's own 8x8 font (parsed from
src/gfx/font8x8_basic.h) over a procedural starfield, next to a framed
gameplay capture. Requires Pillow and docs/images/03-gameplay.png.

Run from the repository root:  python3 tools/gen_banner.py
"""

import os
import random
import re

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

CYAN = (80, 220, 255)
MAGENTA = (235, 80, 255)
YELLOW = (255, 220, 80)
GREY = (150, 150, 170)
DGREY = (90, 90, 110)
BG = (5, 6, 14)


def load_font():
    """Parse the C font table into {codepoint: [8 row bytes]}."""
    src = open(os.path.join(ROOT, "src/gfx/font8x8_basic.h")).read()
    src = re.sub(r"//[^\n]*", "", src)
    body = src[src.index("= {") + 3 : src.rindex("};")]
    glyphs = re.findall(r"\{([^}]*)\}", body)
    font = {}
    for i, g in enumerate(glyphs):
        rows = [int(v.strip(), 16) for v in g.split(",")[:8]]
        font[i] = rows
    return font


FONT = load_font()


def draw_text(img, x, y, text, color, scale):
    px = img.load()
    for ch in text:
        rows = FONT.get(ord(ch), FONT[ord("?")])
        for ry, bits in enumerate(rows):
            for rx in range(8):
                if not bits & (1 << rx):
                    continue
                for sy in range(scale):
                    for sx in range(scale):
                        tx, ty = x + rx * scale + sx, y + ry * scale + sy
                        if 0 <= tx < img.width and 0 <= ty < img.height:
                            px[tx, ty] = color
        x += 8 * scale


def text_w(s, scale):
    return len(s) * 8 * scale


def starfield(img, n, seed=7):
    rnd = random.Random(seed)
    px = img.load()
    for _ in range(n):
        x, y = rnd.randrange(img.width), rnd.randrange(img.height)
        v = rnd.choice([(70, 70, 100), (130, 130, 170), (220, 220, 255)])
        px[x, y] = v
        if v[2] == 255 and y > 0:
            px[x, y - 1] = (130, 130, 170)


def compose(w, h, out, subtitle):
    img = Image.new("RGB", (w, h), BG)
    starfield(img, w * h // 900)

    shot = Image.open(os.path.join(ROOT, "docs/images/03-gameplay.png"))
    sh = int(h * 0.78)
    sw = int(shot.width * sh / shot.height)
    shot = shot.resize((sw, sh), Image.NEAREST)
    sx, sy = w - sw - int(w * 0.045), (h - sh) // 2
    # bezel
    for i, c in ((3, (30, 34, 52)), (1, (80, 220, 255))):
        bez = Image.new("RGB", (sw + 2 * i + 6, sh + 2 * i + 6), c)
        img.paste(bez, (sx - i - 3, sy - i - 3))
    img.paste(shot, (sx, sy))

    left_w = sx
    s_big = max(3, h // 44)
    y0 = int(h * 0.16)
    for line, color in (("NOVA", CYAN), ("RAID", MAGENTA)):
        draw_text(img, (left_w - text_w(line, s_big)) // 2, y0, line, color, s_big)
        y0 += 9 * s_big
    y0 += s_big * 2
    draw_text(img, (left_w - text_w(subtitle, s_big // 3)) // 2, y0,
              subtitle, GREY, s_big // 3)
    y0 += 4 * s_big
    tag = "RASPBERRY PI PICO W ARCADE"
    draw_text(img, (left_w - text_w(tag, s_big // 3)) // 2, y0,
              tag, DGREY, s_big // 3)

    img.save(os.path.join(ROOT, out))
    print("wrote", out)


if __name__ == "__main__":
    compose(1280, 400, "docs/images/banner.png", "DEEP SPACE DEFENSE")
    compose(1280, 640, "docs/images/social-preview.png", "DEEP SPACE DEFENSE")
