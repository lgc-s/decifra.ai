#!/usr/bin/env python3
"""Reproduz em PNG os comandos de desenho registrados pelo stub (tests/stub/raylib_stub.c).
Serve para conferir layout das telas sem a Raylib. A fonte aqui é só aproximada;
na Raylib real o texto usa a fonte padrão dela.

    python3 tools/render_dump.py dump_60.txt saida.png [escala]
"""
import sys
from PIL import Image, ImageDraw, ImageChops, ImageFont
import math

W, H = 384, 216
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf"
_fonts = {}

def font(size):
    if size not in _fonts:
        _fonts[size] = ImageFont.truetype(FONT, max(6, round(size * 0.92)))
    return _fonts[size]

# Larguras dos glifos da fonte padrão da Raylib 5.0 (rtext.c), para o texto ocupar o mesmo espaço do jogo real.
CHARS_WIDTH = [
    3,1,4,6,5,7,6,2,3,3,5,5,2,4,1,7,5,2,5,5,5,5,5,5,5,5,1,1,3,4,3,6,
    7,6,6,6,6,6,6,6,6,3,5,6,5,7,6,6,6,6,6,6,7,6,7,7,6,6,6,2,7,2,3,5,
    2,5,5,5,5,5,4,5,5,1,2,5,2,5,5,5,5,5,5,5,4,5,5,5,5,5,5,3,1,3,4,4,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,5,5,5,7,1,5,3,7,3,5,4,1,7,4,3,5,3,3,2,5,6,1,2,2,3,5,6,6,6,6,
    6,6,6,6,6,6,7,6,6,6,6,6,3,3,3,3,7,6,6,6,6,6,6,5,6,6,6,6,6,6,4,6,
    5,5,5,5,5,5,9,5,5,5,5,5,2,2,3,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,3,5]

def draw_text(d, x, y, text, size, fill):
    size = max(size, 10)
    scale, spacing = size / 10.0, size // 10
    f = font(size)
    for ch in text:
        idx = ord(ch) - 32
        if idx < 0 or idx >= 224:
            idx = 63 - 32
        w = CHARS_WIDTH[idx] * scale
        if ch != " ":
            gw = f.getlength(ch)
            ox = (w - gw) / 2.0                       # centraliza o glifo na largura da fonte do jogo
            d.text((x + 1 + ox, y + 1), ch, font=f, fill=(0, 0, 0, 90))
            d.text((x + ox, y), ch, font=f, fill=fill)
        x += w + spacing

def render(path, out, scale=3):
    base = Image.new("RGB", (W, H), (0, 0, 0))
    d = ImageDraw.Draw(base, "RGBA")
    additive = False

    def target():
        if not additive:
            return d, None
        layer = Image.new("RGB", (W, H), (0, 0, 0))
        return ImageDraw.Draw(layer, "RGBA"), layer

    for line in open(path, encoding="utf-8"):
        line = line.rstrip("\n")
        if line.startswith("TX "):
            head, text = line[3:].split("|", 1)
            x, y, s, r, g, b, a = map(int, head.split())
            draw_text(d, x, y, text, s, (r, g, b, a))
            continue
        parts = line.split()
        cmd, v = parts[0], parts[1:]
        if cmd == "BM":
            additive = v[0] == "1"
            continue
        dd, layer = target()
        if cmd == "R":
            x, y, w, h, r, g, b, a = map(int, v)
            if w > 0 and h > 0:
                dd.rectangle([x, y, x + w - 1, y + h - 1], fill=(r, g, b, a))
        elif cmd == "RL":
            x, y, w, h, r, g, b, a = map(int, v)
            dd.rectangle([x, y, x + w - 1, y + h - 1], outline=(r, g, b, a))
        elif cmd == "GV":
            x, y, w, h = map(int, v[:4]); c1 = list(map(int, v[4:8])); c2 = list(map(int, v[8:12]))
            for i in range(h):
                t = i / max(1, h - 1)
                col = tuple(int(c1[k] + (c2[k] - c1[k]) * t) for k in range(4))
                dd.rectangle([x, y + i, x + w - 1, y + i], fill=col)
        elif cmd == "C":
            x, y = int(v[0]), int(v[1]); r = float(v[2]); col = tuple(map(int, v[3:7]))
            dd.ellipse([x - r, y - r, x + r, y + r], fill=col)
        elif cmd == "T":
            p = list(map(float, v[:6])); col = tuple(map(int, v[6:10]))
            dd.polygon([(p[0], p[1]), (p[2], p[3]), (p[4], p[5])], fill=col)
        elif cmd == "P":
            cx, cy = float(v[0]), float(v[1]); n = int(v[2]); r = float(v[3]); rot = float(v[4]); col = tuple(map(int, v[5:9]))
            pts = [(cx + r * math.cos(math.radians(rot + 360.0 / n * i)), cy + r * math.sin(math.radians(rot + 360.0 / n * i))) for i in range(n)]
            dd.polygon(pts, fill=col)
        if layer is not None:
            base_new = ImageChops.add(base, layer)
            base.paste(base_new)
    base.resize((W * scale, H * scale), Image.NEAREST).save(out)

if __name__ == "__main__":
    render(sys.argv[1], sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 3)
